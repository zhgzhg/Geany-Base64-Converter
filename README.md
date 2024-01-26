Base64 Converter for Geany
==========================

Base64 Converter is a plugin for Geany used to convert
from and to base64 strings.

This repository represents an independent project whose results could
be manually integrated with Geany.

* Supported platforms: Linux
* License: MIT

Features:

* Conversion from/to base64 strings.
* Shows results in 'Messages' and potentially in a popup window.
* Use only the current selected text or use the whole document for
conversion.

Dependencies:

* geany
* geany-devel or geany-common  (depending on the distro)
* gtk+3.0-dev(el) or gtk+2.0-dev(el)  (depending on the distro)
* libb64-dev(el)
* make
* pkg-config

Compilation
-----------

To compile run: `make`

To install (root privileges needed) run: `sudo make install`

To uninstall (root privileges needed) run: `sudo make uninstall`

Local to the current account installation
-----------------------------------------

This is an alternative to globally install the plugin for all users. No root privileges needed.

To install for the current account run: `make localinstall`

To uninstall for the current account run: `make localuninstall`

Other notes
-----------

Attention MacOS users - this plugin will work with the manually
installed and compiled Geany editor from source code. It will not work
with the version installed from dmg files.

Other Useful Plugins
--------------------
* [Geany JSON Prettifier](https://github.com/zhgzhg/Geany-JSON-Prettifier)
* [Geany Generic SQL Formatter](https://github.com/zhgzhg/Geany-Generic-SQL-Formatter)
* [Geanny Unix Timestamp Converter](https://github.com/zhgzhg/Geany-Unix-Timestamp-Converter)
