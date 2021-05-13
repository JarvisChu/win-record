{
	"targets": [
		{
			"target_name": "win_record_addon",
			"sources": ["src/record.cc", "src/addon.cc",],
			"include_dirs": [
				"<!(node -e \"require('nan')\")"
			]
		}
	]
}
