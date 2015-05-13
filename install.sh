sudo /etc/init.d/lighttpd stop
cp ./dist/Release/GNU-Linux-x86/hydrogate /home/hydrogate/www/bin/
cp configfastcgi.js /home/hydrogate/www/bin/
sudo /etc/init.d/lighttpd start
