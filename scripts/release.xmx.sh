#/bin/sh
echo XMX Release : copy percmd , resources and trax host to releases/xmx
rm -rf releases/xmx
mkdir releases/xmx

cp ../PercCmd/releases/xmx/* releases/xmx/
cp resources/*.txt releases/xmx/
cp resources/xmx/* releases/xmx/

cp build.xmx/TraxHost releases/xmx/
cp build.xmx/rtaudio/librtaudio.so.7 releases/xmx/
/opt/homebrew/bin/aarch64-elf-strip --strip-unneeded releases/xmx/TraxHost

cd releases/xmx
rm ../traxhost_xmx.zip
zip -r ../traxhost_xmx.zip .
cd ../..

