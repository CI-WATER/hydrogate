sudo /etc/init.d/lighttpd stop
cp ./dist/Release/GNU-Linux-x86/hydrogate /home/demouser/www/bin/
cp configfastcgi.js /home/demouser/www/bin/
sudo /etc/init.d/lighttpd start
