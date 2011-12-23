Changes from tarball:

	# Line 261 of wm.c, changed "CARD32" to "CARD64" to prevent a runtime error when activating on 64-bit system.
		Will need to detect 32/64 bit at compile time somehow.

TODO:

	# Rework triggering code so that activation can be triggered by command rather than just a hotkey.
	  Better to use DE's global hotkeys.
