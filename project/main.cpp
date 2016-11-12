/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <initializer_list>
#include <jni.h>
#include <errno.h>
#include <cassert>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include "third_party/android_native_app_glue.h" // modified to point to local version

// These headers cause a problem when included:
#include "firebase/admob.h"
#include "firebase/admob/rewarded_video.h"
#include "firebase/admob/types.h"
#include "firebase/app.h"
#include "firebase/future.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native_activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native_activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native_activity", __VA_ARGS__))

// App Id and key.
const char* kAppID = "APP_ID";
const char* kApiKey = "API_KEY";
const char* kAdMobAppID = "ADD_MOB_ID";

// Test add ids.
const char* kBannerAdUnit = "ca-app-pub-3940256099942544/6300978111";
const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/1033173712";

// Test device Ids.
// To find the id for a specific device, run the app, do a logcat and grep
// for "Ads". There will be a message printing the device id.
const char* kTestDeviceIDs[] = {
  "TEST_DEVICE_ID"
};

struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;
};

bool ProcessEvents(struct android_app* state, int msec) {
  struct android_poll_source* source = nullptr;
  int events;
  int looperId = ALooper_pollAll(msec, nullptr, &events,
                                 reinterpret_cast<void**>(&source));
  if (looperId >= 0 && source) {
    source->process(state, source);
  }
  return false;
}

void WaitForFutureCompletion(
    struct android_app* state, firebase::FutureBase future) {
  while (!ProcessEvents(state, 1000)) {
    if (future.Status() != firebase::kFutureStatusPending) {
      break;
    }
  }

  if (future.Error() != firebase::admob::kAdMobErrorNone) {
    LOGE("ERROR: Action failed with error code %d and message \"%s\".",
         future.Error(), future.ErrorMessage());
  }
}

void init_admob(struct android_app* state) {
  JavaVM* vm = state->activity->vm;
  JNIEnv* env = NULL;
  jint result = vm->AttachCurrentThread(&env, NULL);
  if (result != JNI_OK) {
    return;
  }
  LOGI("Attached thread");

  firebase::AppOptions options;
  options.set_app_id(kAppID);
  options.set_api_key(kApiKey);
  firebase::App* firebase_app = firebase::App::Create(
      options, env, state->activity->clazz);
  LOGI("Created firebase app");

  firebase::admob::Initialize(*firebase_app, kAdMobAppID);
  LOGI("Initialized firebase app");

  firebase::admob::rewarded_video::Initialize();
  WaitForFutureCompletion(state, firebase::admob::rewarded_video::InitializeLastResult());
  LOGI("Initialized rewarded video");
}

void release_admob(struct android_app* state) {
  firebase::admob::rewarded_video::Destroy();

  state->activity->vm->DetachCurrentThread();
}

firebase::admob::AdRequest get_request() {
  firebase::admob::AdRequest request;
  request.gender = firebase::admob::kGenderUnknown;
  request.tagged_for_child_directed_treatment =
      firebase::admob::kChildDirectedTreatmentStateTagged;
  request.birthday_day = 10;
  request.birthday_month = 11;
  request.birthday_year = 1976;
  request.keyword_count = 0;
  request.extras_count = 0;
  request.test_device_id_count = sizeof(kTestDeviceIDs) / sizeof(kTestDeviceIDs[0]);
  request.test_device_ids = kTestDeviceIDs;

  return request;
}

void load_banner_ad(struct android_app* state) {
  firebase::admob::BannerView* banner_view;
  banner_view = new firebase::admob::BannerView();

  firebase::admob::AdSize ad_size;
  ad_size.ad_size_type = firebase::admob::kAdSizeStandard;
  ad_size.width = 320;
  ad_size.height = 50;
  banner_view->Initialize(state->activity->clazz, kBannerAdUnit, ad_size);
  WaitForFutureCompletion(state, banner_view->InitializeLastResult());

  LOGI("Loading a banner ad.");
  banner_view->LoadAd(get_request());
  WaitForFutureCompletion(state, banner_view->LoadAdLastResult());

  LOGI("Showing the banner ad.");
  banner_view->Show();
  WaitForFutureCompletion(state, banner_view->ShowLastResult());
}

void load_interstitial_ad(struct android_app* state) {
  LOGW("Creating the InterstitialAd.");
  firebase::admob::InterstitialAd* interstitial =
      new firebase::admob::InterstitialAd();
  interstitial->Initialize(state->activity->clazz, kInterstitialAdUnit);
  WaitForFutureCompletion(state, interstitial->InitializeLastResult());

  LOGW("Loading an interstitial ad.");
  interstitial->LoadAd(get_request());
  WaitForFutureCompletion(state, interstitial->LoadAdLastResult());

  LOGW("Showing the interstitial ad.");
  interstitial->Show();
  WaitForFutureCompletion(state, interstitial->ShowLastResult());

  while (interstitial->GetPresentationState() !=
         firebase::admob::InterstitialAd::PresentationState::
             kPresentationStateHidden) {
    ProcessEvents(state, 1000);
  }
}

void load_rewarded_video(struct android_app* state) {
  firebase::admob::rewarded_video::LoadAd(kInterstitialAdUnit, get_request());
  LOGI("Loaded rewarded video");

  LOGI("Showing rewarded video");
  firebase::admob::rewarded_video::Show(state->activity->clazz);
  WaitForFutureCompletion(state, firebase::admob::rewarded_video::ShowLastResult());
  LOGI("Showed rewarded video");
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, dummy, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    /* Here, the application chooses the configuration it desires.
     * find the best match if possible, otherwise use the very first one
     */
    eglChooseConfig(display, attribs, nullptr,0, &numConfigs);
    auto supportedConfigs = new EGLConfig[numConfigs];
    assert(supportedConfigs);
    eglChooseConfig(display, attribs, supportedConfigs, numConfigs, &numConfigs);
    assert(numConfigs);
    auto i = 0;
    for (; i < numConfigs; i++) {
        auto& cfg = supportedConfigs[i];
        EGLint r, g, b, d;
        if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r)   &&
            eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
            eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b)  &&
            eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) &&
            r == 8 && g == 8 && b == 8 && d == 0 ) {

            config = supportedConfigs[i];
            break;
        }
    }
    if (i == numConfigs) {
        config = supportedConfigs[0];
    }

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, NULL);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    // Check openGL on the system
    auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOGI("OpenGL Info: %s", info);
    }
    // Initialize GL state.
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);

    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
        return;
    }

    // Just fill the screen with a color.
    glClearColor(((float)engine->state.x)/engine->width, engine->state.angle,
                 ((float)engine->state.y)/engine->height, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                               engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                               engine->accelerometerSensor,
                                               (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine;

    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(
                                        engine.sensorManager,
                                        ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(
                                    engine.sensorManager,
                                    state->looper, LOOPER_ID_USER,
                                    NULL, NULL);

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    init_admob(state);
    load_interstitial_ad(state);
    load_rewarded_video(state);
    load_banner_ad(state);

    // loop waiting for stuff to do.

    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                                      (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                       &event, 1) > 0) {
                        // LOGI("accelerometer: x=%f y=%f z=%f",
                        //      event.acceleration.x, event.acceleration.y,
                        //      event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                release_admob(state);
                return;
            }
        }

        if (engine.animating) {
            // Done with events; draw next animation frame.
            engine.state.angle += .01f;
            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            }

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }
}
//END_INCLUDE(all)
