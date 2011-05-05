push :
	cp -p *.php settings.inc trends.inc *.js *.css favicon.ico ~/httparchive.org/.
	/bin/rm ~/httparchive.org/admin.php
	mkdir -p ~/httparchive.org/images
	cp -p images/*.* ~/httparchive.org/images/.
	mkdir -p ~/httparchive.org/lists
	cp -p lists/*.txt ~/httparchive.org/lists/.
	mkdir -p ~/httparchive.org/cache
	chmod 777 ~/httparchive.org/cache
	mkdir -p ~/httparchive.org/downloads
	cp -pu downloads/*.gz ~/httparchive.org/downloads/.
	echo "push .htaccess!"
	echo "CREATE interesting-images.js.cache !!!!!!!!!!!"

pushmobile :
	cp -p *.php settings.inc trends.inc *.js *.css favicon.ico ~/mobile.httparchive.org/.
	/bin/rm ~/mobile.httparchive.org/admin.php
	mkdir -p ~/mobile.httparchive.org/images
	cp -p images/*.* ~/mobile.httparchive.org/images/.
	mkdir -p ~/mobile.httparchive.org/lists
	cp -p lists/*.txt ~/mobile.httparchive.org/lists/.
	mkdir -p ~/mobile.httparchive.org/cache
	chmod 777 ~/mobile.httparchive.org/cache
	mkdir -p ~/mobile.httparchive.org/downloads
	cp -pu downloads/*.gz ~/mobile.httparchive.org/downloads/.
	echo "push .htaccess!"
	echo "CREATE interesting-images.js.cache !!!!!!!!!!!"

pushbeta :
	mkdir -p ~/beta.httparchive.org
	cp -p *.php settings.inc trends.inc *.js *.css favicon.ico .htaccess ~/beta.httparchive.org/.
	/bin/rm ~/beta.httparchive.org/admin.php
	mkdir -p ~/beta.httparchive.org/images
	cp -p images/*.* ~/beta.httparchive.org/images/.
	mkdir -p ~/beta.httparchive.org/lists
	cp -p lists/*.txt ~/beta.httparchive.org/lists/.
	mkdir -p ~/beta.httparchive.org/downloads
	cp -pu downloads/*.gz ~/beta.httparchive.org/downloads/.
	php cachegen.php
	mkdir -p ~/beta.httparchive.org/cache
	chmod 777 ~/beta.httparchive.org/cache
	cp -pu cache/interesting* ~/beta.httparchive.org/cache/.
