{
	"targets": [
		{
			"target_name": "win_record_addon",
			"sources": ["src/record.cc", "src/addon.cc",],
			"include_dirs": [
        		"<!@(node -p \"require('node-addon-api').include\")"
      		],
      		'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
		}
	]
}
