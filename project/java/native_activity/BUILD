android_binary(
    name = 'myapp',
    legacy_native_support = 0,
    manifest = 'AndroidManifest.xml',
    deps = [
        '//project:main',
	'@androidsdk//com.google.firebase:firebase-ads-9.6.1'
    ],
    manifest_values = {'applicationId': 'mybazelapp.android'},
    resource_files = glob(['res/**']),

    assets = glob(['assets/**']),
    assets_dir = 'assets'
)
