all: undernet.dylib dbtool

undernet.dylib: undernet.c oddsock
	gcc -Wall -dynamiclib -o undernet.dylib undernet.c -lcurl -levent_core -levent_extra

install: undernet.dylib exec.sh com.phyrrus9.oddsockspatch.plist oddsock dbtool
	sudo mkdir -p /usr/local/undernet
	sudo cp undernet.dylib /usr/local/undernet
	sudo cp exec.sh /usr/local/undernet
	sudo cp oddsock /usr/local/undernet
	sudo cp com.phyrrus9.oddsockspatch.plist /System/Library/LaunchDaemons/
	sudo cp dbtool /usr/bin/udbtool

uninstall: /System/Library/LaunchDaemons/com.phyrrus9.oddsockspatch.plist
	sudo rm /System/Library/LaunchDaemons/com.phyrrus9.oddsockspatch.plist
	sudo rm -rf /usr/local/undernet
	sudo rm /usr/bin/udbtool

oddsock:
	rm -rf oddsock-read-only
	svn checkout http://oddsock.googlecode.com/svn/trunk/ oddsock-read-only
	make -C ./oddsock-read-only
	cp ./oddsock-read-only/oddsock ./
dbtool: dbtool.c
	gcc -o dbtool dbtool.c

clean:
	rm -f dbtool
	rm -f undernet.dylib
	rm -rf ./oddsock-read-only
	rm -f oddsock
