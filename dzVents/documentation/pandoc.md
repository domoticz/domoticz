#How to generate wiki docs based on README.md so it can be pasted in Domoticz wiki?

Make sure you have pandoc installed, then run in this folder:

pandoc -r markdown README.md -t mediawiki -o README.wiki