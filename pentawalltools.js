pentawalltools = require('./build/Release/pentawalltools.node');
SlowBuffer = require('buffer').SlowBuffer;
Buffer = require('buffer').Buffer;


for (var key in pentawalltools) {
	var val = pentawalltools[key];
	SlowBuffer.prototype[key] = val;
	Buffer.prototype[key] = val;
	exports[key] = val;
}

