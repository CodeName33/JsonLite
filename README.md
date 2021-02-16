# JsonLite
Low memory Json parser for arduino and etc.

# Why JsonLite?
This library uses low memory. It doesn't build trees or something while parsing json, Strings will be created only when you get values from nodes. It works with pointers to json string or json PROGMEM string. Yes you can parse PROGMEM json srting directly without loading to ram.
