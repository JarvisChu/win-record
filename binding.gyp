{
	"targets": [
		{
			"target_name": "win_record_addon",
			"sources": [
				"src/record.cc", 
				"src/addon.cc", 
				"src/audio_processor.cc",
				"src/silk_encoder.cc", 
				'<!@(ls -1 src/silk/src/*.c)'
			],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"<(module_root_dir)/src",
				"<(module_root_dir)/src/silk/interface",
			]
		}
	]
}
