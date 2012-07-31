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

template <class Derived> struct BinaryAction {
	Handle<Value> apply(Handle<Object>& buffer, const uint8_t* data, size_t size, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		HandleScope scope;

		Local<Object> self = args.This();
		if (!Buffer::HasInstance(self)) {
			return ThrowException(Exception::TypeError(String::New(
					"Argument should be a buffer object.")));
		}

		if (args[0]->IsString()) {
			String::Utf8Value s(args[0]->ToString());
			return static_cast<Derived*>(this)->apply(self, (uint8_t*) *s, s.length(), args, scope);
		}
		if (Buffer::HasInstance(args[0])) {
			Local<Object> other = args[0]->ToObject();
			return static_cast<Derived*>(this)->apply(
					self, (const uint8_t*) Buffer::Data(other), Buffer::Length(other), args, scope);
		}

		static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New(
				"Second argument must be a string or a buffer."));
		return ThrowException(Exception::TypeError(illegalArgumentException));
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

inline Handle<Value> decodeG3d2EncodingWithAlpha(const uint8_t* const data, const size_t size, const uint8_t* const data2, const size_t size2,const Arguments& args, HandleScope& scope) {
	if (size & 1) {
		return ThrowException(Exception::Error(String::New(
				"Odd string length, this is not hexadecimal data.")));
	}

	if (size == 0) {
		return String::Empty();
	}

	Handle<Object>& buffer = Buffer::New(size / 4)->handle_;

	uint8_t *src = (uint8_t *) data;
	uint8_t *bg = (uint8_t *) data2;
	uint8_t *dst = (uint8_t *) (const uint8_t*) Buffer::Data(buffer);

	for (size_t i = 0; i < size; i += 4) {
		int a = fromHexTable[*src++];
		int aa = fromHexTable[*src++];
		int b = fromHexTable[*src++];
		int ba = fromHexTable[*src++];

		if (a == -1 || b == -1 || aa == -1 || ba == -1) {
			return ThrowException(Exception::Error(String::New(
					"This is not hexadecimal data.")));
		}
		
		int old = *bg++;


		int old_a = old & 0x0f;
		int old_b = old >> 4;
		
		int result_a  = ((old_a * (15-aa) + a*aa) >> 4) & 0xf;
		int result_b  = ((old_b * (15-ba) + b*ba) >> 4) & 0xf;


		*dst++ = result_a | (result_b << 4);
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
struct FromG3d2EncodingWithAlphaAction: BinaryAction<FromG3d2EncodingWithAlphaAction> {
	Handle<Value> apply(Handle<Object>& buffer, const uint8_t* data2, size_t length2,const Arguments& args, HandleScope& scope) {
		const uint8_t* data = (const uint8_t*) Buffer::Data(buffer);
		size_t length = Buffer::Length(buffer);
		return decodeG3d2EncodingWithAlpha(data, length, data2, length2,args, scope);
	}
};

Handle<Value> FromG3d2Encoding(const Arguments& args) {
	return FromG3d2EncodingAction()(args);
}
Handle<Value> FromG3d2EncodingWithAlpha(const Arguments& args) {
	return FromG3d2EncodingWithAlphaAction()(args);
}

void RegisterModule(Handle<Object> target) {
	target->Set(String::NewSymbol("fromG3d2Encoding"), FunctionTemplate::New(FromG3d2Encoding)->GetFunction());
	target->Set(String::NewSymbol("fromG3d2EncodingWithAlpha"), FunctionTemplate::New(FromG3d2EncodingWithAlpha)->GetFunction());
}

}

NODE_MODULE(pentawalltools, RegisterModule)

