#include "record.h"

NODE_MODULE_INIT() {
	Record::Initialize(exports, module, context);
}