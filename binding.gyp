{
	"targets": [
		{
			"target_name": "win_record_addon",
			"sources": ["src/record.cc", "src/addon.cc", "src/audio_processor.cc"],
			"include_dirs": [
				"<!(node -e \"require('nan')\")"
			]
		}
	]
}
