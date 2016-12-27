all: cgi-env/TestService.dll

cgi-env/TestService.dll: test.cs
	mcs /target:library -r:../../zoo-api/mono/ZMaps.dll -out:cgi-env/TestService.dll test.cs 

clean: 
	rm -rf cgi-env/*dll
