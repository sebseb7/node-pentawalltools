#include <node.h>
#include <node_buffer.h>

using namespace v8;
using namespace node;


namespace {

template <class Derived> struct UnaryAction {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		HandleScope scope;

		Local<Object> self = args.This();
		if (!Buffer::HasInstance(self)) {
			return ThrowException(Exception::TypeError(String::New(
					"Argument should be a buffer object.")));
		}

		return static_cast<Derived*>(this)->apply(self, args, scope);
	}
};

static char fromHexTable[] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,
		10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

inline Handle<Value> decodeG3d2Encoding(const uint8_t* const data, const size_t size, const Arguments& args, HandleScope& scope) {
	if (size & 1) {
		return ThrowException(Exception::Error(String::New(
				"Odd string length, this is not hexadecimal data.")));
	}

	if (size == 0) {
		return String::Empty();
	}

	Handle<Object>& buffer = Buffer::New(size / 2)->handle_;

	uint8_t *src = (uint8_t *) data;
	uint8_t *dst = (uint8_t *) (const uint8_t*) Buffer::Data(buffer);

	for (size_t i = 0; i < size; i += 2) {
		int a = fromHexTable[*src++];
		int b = fromHexTable[*src++];

		if (a == -1 || b == -1) {
			return ThrowException(Exception::Error(String::New(
					"This is not hexadecimal data.")));
		}

		*dst++ = a | (b << 4);
	}

	return buffer;
}

struct FromG3d2EncodingAction: UnaryAction<FromG3d2EncodingAction> {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope) {
		const uint8_t* data = (const uint8_t*) Buffer::Data(buffer);
		size_t length = Buffer::Length(buffer);
		return decodeG3d2Encoding(data, length, args, scope);
	}
};

Handle<Value> FromG3d2Encoding(const Arguments& args) {
	return FromG3d2EncodingAction()(args);
}

void RegisterModule(Handle<Object> target) {
	target->Set(String::NewSymbol("fromG3d2Encoding"), FunctionTemplate::New(FromG3d2Encoding)->GetFunction());
}

}

NODE_MODULE(pentawalltools, RegisterModule)

