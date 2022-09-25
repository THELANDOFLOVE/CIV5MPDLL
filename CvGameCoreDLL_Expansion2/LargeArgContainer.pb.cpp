// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: LargeArgContainer.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "LargeArgContainer.pb.h"
#include <CvGameCoreDLLPCH.h>
#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
// @@protoc_insertion_point(includes)

void protobuf_ShutdownFile_LargeArgContainer_2eproto() {
    delete BasicArguments::default_instance_;
    delete LargeArgContainer::default_instance_;
}

#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
void protobuf_AddDesc_LargeArgContainer_2eproto_impl() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

#else
void protobuf_AddDesc_LargeArgContainer_2eproto() {
    static bool already_here = false;
    if (already_here) return;
    already_here = true;
    GOOGLE_PROTOBUF_VERIFY_VERSION;

#endif
    BasicArguments::default_instance_ = new BasicArguments();
    LargeArgContainer::default_instance_ = new LargeArgContainer();
    BasicArguments::default_instance_->InitAsDefaultInstance();
    LargeArgContainer::default_instance_->InitAsDefaultInstance();
    ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_LargeArgContainer_2eproto);
}

#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AddDesc_LargeArgContainer_2eproto_once_);
void protobuf_AddDesc_LargeArgContainer_2eproto() {
    ::google::protobuf::::google::protobuf::GoogleOnceInit(&protobuf_AddDesc_LargeArgContainer_2eproto_once_,
        &protobuf_AddDesc_LargeArgContainer_2eproto_impl);
}
#else
// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_LargeArgContainer_2eproto {
    StaticDescriptorInitializer_LargeArgContainer_2eproto() {
        protobuf_AddDesc_LargeArgContainer_2eproto();
    }
} static_descriptor_initializer_LargeArgContainer_2eproto_;
#endif

// ===================================================================

#ifndef _MSC_VER
const int BasicArguments::kArgTypeFieldNumber;
const int BasicArguments::kIdentifier1FieldNumber;
const int BasicArguments::kIdentifier2FieldNumber;
const int BasicArguments::kLongMessageFieldNumber;
#endif  // !_MSC_VER

BasicArguments::BasicArguments()
    : ::google::protobuf::MessageLite() {
    SharedCtor();
}

void BasicArguments::InitAsDefaultInstance() {
}

BasicArguments::BasicArguments(const BasicArguments & from)
    : ::google::protobuf::MessageLite() {
    SharedCtor();
    MergeFrom(from);
}

void BasicArguments::SharedCtor() {
    _cached_size_ = 0;
    argtype_ = const_cast<::std::string*>(&::google::protobuf::internal::kEmptyString);
    identifier1_ = 0;
    identifier2_ = 0;
    longmessage_ = const_cast<::std::string*>(&::google::protobuf::internal::kEmptyString);
    ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

BasicArguments::~BasicArguments() {
    SharedDtor();
}

void BasicArguments::SharedDtor() {
    if (argtype_ != &::google::protobuf::internal::kEmptyString) {
        delete argtype_;
    }
    if (longmessage_ != &::google::protobuf::internal::kEmptyString) {
        delete longmessage_;
    }
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
    if (this != &default_instance()) {
#else
    if (this != default_instance_) {
#endif
    }
    }

void BasicArguments::SetCachedSize(int size) const {
    GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
    _cached_size_ = size;
    GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const BasicArguments& BasicArguments::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
    protobuf_AddDesc_LargeArgContainer_2eproto();
#else
    if (default_instance_ == NULL) protobuf_AddDesc_LargeArgContainer_2eproto();
#endif
    return *default_instance_;
}

BasicArguments* BasicArguments::default_instance_ = NULL;

BasicArguments* BasicArguments::New() const {
    return new BasicArguments;
}

void BasicArguments::Clear() {
    if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
        if (has_argtype()) {
            if (argtype_ != &::google::protobuf::internal::kEmptyString) {
                argtype_->clear();
            }
        }
        identifier1_ = 0;
        identifier2_ = 0;
        if (has_longmessage()) {
            if (longmessage_ != &::google::protobuf::internal::kEmptyString) {
                longmessage_->clear();
            }
        }
    }
    ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

bool BasicArguments::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream * input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
    ::google::protobuf::uint32 tag;
    while ((tag = input->ReadTag()) != 0) {
        switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
            // required string argType = 1;
        case 1: {
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
                DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                    input, this->mutable_argtype()));
            }
            else {
                goto handle_uninterpreted;
            }
            if (input->ExpectTag(16)) goto parse_identifier1;
            break;
        }

              // optional int32 identifier1 = 2;
        case 2: {
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
            parse_identifier1:
                DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                    ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                        input, &identifier1_)));
                set_has_identifier1();
            }
            else {
                goto handle_uninterpreted;
            }
            if (input->ExpectTag(24)) goto parse_identifier2;
            break;
        }

              // optional int32 identifier2 = 3;
        case 3: {
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
            parse_identifier2:
                DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                    ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                        input, &identifier2_)));
                set_has_identifier2();
            }
            else {
                goto handle_uninterpreted;
            }
            if (input->ExpectTag(34)) goto parse_longMessage;
            break;
        }

              // optional string longMessage = 4;
        case 4: {
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
            parse_longMessage:
                DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                    input, this->mutable_longmessage()));
            }
            else {
                goto handle_uninterpreted;
            }
            if (input->ExpectAtEnd()) return true;
            break;
        }

        default: {
        handle_uninterpreted:
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
                return true;
            }
            DO_(::google::protobuf::internal::WireFormatLite::SkipField(input, tag));
            break;
        }
        }
    }
    return true;
#undef DO_
}

void BasicArguments::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream * output) const {
    // required string argType = 1;
    if (has_argtype()) {
        ::google::protobuf::internal::WireFormatLite::WriteString(
            1, this->argtype(), output);
    }

    // optional int32 identifier1 = 2;
    if (has_identifier1()) {
        ::google::protobuf::internal::WireFormatLite::WriteInt32(2, this->identifier1(), output);
    }

    // optional int32 identifier2 = 3;
    if (has_identifier2()) {
        ::google::protobuf::internal::WireFormatLite::WriteInt32(3, this->identifier2(), output);
    }

    // optional string longMessage = 4;
    if (has_longmessage()) {
        ::google::protobuf::internal::WireFormatLite::WriteString(
            4, this->longmessage(), output);
    }

}

int BasicArguments::ByteSize() const {
    int total_size = 0;

    if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
        // required string argType = 1;
        if (has_argtype()) {
            total_size += 1 +
                ::google::protobuf::internal::WireFormatLite::StringSize(
                    this->argtype());
        }

        // optional int32 identifier1 = 2;
        if (has_identifier1()) {
            total_size += 1 +
                ::google::protobuf::internal::WireFormatLite::Int32Size(
                    this->identifier1());
        }

        // optional int32 identifier2 = 3;
        if (has_identifier2()) {
            total_size += 1 +
                ::google::protobuf::internal::WireFormatLite::Int32Size(
                    this->identifier2());
        }

        // optional string longMessage = 4;
        if (has_longmessage()) {
            total_size += 1 +
                ::google::protobuf::internal::WireFormatLite::StringSize(
                    this->longmessage());
        }

    }
    GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
    _cached_size_ = total_size;
    GOOGLE_SAFE_CONCURRENT_WRITES_END();
    return total_size;
}

void BasicArguments::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite & from) {
    MergeFrom(*::google::protobuf::down_cast<const BasicArguments*>(&from));
}

void BasicArguments::MergeFrom(const BasicArguments & from) {
    GOOGLE_CHECK_NE(&from, this);
    if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
        if (from.has_argtype()) {
            set_argtype(from.argtype());
        }
        if (from.has_identifier1()) {
            set_identifier1(from.identifier1());
        }
        if (from.has_identifier2()) {
            set_identifier2(from.identifier2());
        }
        if (from.has_longmessage()) {
            set_longmessage(from.longmessage());
        }
    }
}

void BasicArguments::CopyFrom(const BasicArguments & from) {
    if (&from == this) return;
    Clear();
    MergeFrom(from);
}

bool BasicArguments::IsInitialized() const {
    if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

    return true;
}

void BasicArguments::Swap(BasicArguments * other) {
    if (other != this) {
        std::swap(argtype_, other->argtype_);
        std::swap(identifier1_, other->identifier1_);
        std::swap(identifier2_, other->identifier2_);
        std::swap(longmessage_, other->longmessage_);
        std::swap(_has_bits_[0], other->_has_bits_[0]);
        std::swap(_cached_size_, other->_cached_size_);
    }
}

::std::string BasicArguments::GetTypeName() const {
    return "BasicArguments";
}


// ===================================================================

#ifndef _MSC_VER
const int LargeArgContainer::kFunctionToCallFieldNumber;
const int LargeArgContainer::kInvokeStampFieldNumber;
const int LargeArgContainer::kArgsFieldNumber;
#endif  // !_MSC_VER

LargeArgContainer::LargeArgContainer()
    : ::google::protobuf::MessageLite() {
    SharedCtor();
}

void LargeArgContainer::InitAsDefaultInstance() {
}

LargeArgContainer::LargeArgContainer(const LargeArgContainer & from)
    : ::google::protobuf::MessageLite() {
    SharedCtor();
    MergeFrom(from);
}

void LargeArgContainer::SharedCtor() {
    _cached_size_ = 0;
    functiontocall_ = const_cast<::std::string*>(&::google::protobuf::internal::kEmptyString);
    invokestamp_ = 0;
    ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LargeArgContainer::~LargeArgContainer() {
    SharedDtor();
}

void LargeArgContainer::SharedDtor() {
    if (functiontocall_ != &::google::protobuf::internal::kEmptyString) {
        delete functiontocall_;
    }
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
    if (this != &default_instance()) {
#else
    if (this != default_instance_) {
#endif
    }
    }

void LargeArgContainer::SetCachedSize(int size) const {
    GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
    _cached_size_ = size;
    GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const LargeArgContainer& LargeArgContainer::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
    protobuf_AddDesc_LargeArgContainer_2eproto();
#else
    if (default_instance_ == NULL) protobuf_AddDesc_LargeArgContainer_2eproto();
#endif
    return *default_instance_;
}

LargeArgContainer* LargeArgContainer::default_instance_ = NULL;

LargeArgContainer* LargeArgContainer::New() const {
    return new LargeArgContainer;
}

void LargeArgContainer::Clear() {
    if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
        if (has_functiontocall()) {
            if (functiontocall_ != &::google::protobuf::internal::kEmptyString) {
                functiontocall_->clear();
            }
        }
        invokestamp_ = 0;
    }
    args_.Clear();
    ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

bool LargeArgContainer::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream * input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
    ::google::protobuf::uint32 tag;
    while ((tag = input->ReadTag()) != 0) {
        switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
            // required string functionToCall = 1;
        case 1: {
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
                DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                    input, this->mutable_functiontocall()));
            }
            else {
                goto handle_uninterpreted;
            }
            if (input->ExpectTag(16)) goto parse_invokeStamp;
            break;
        }

              // optional int32 invokeStamp = 2;
        case 2: {
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
            parse_invokeStamp:
                DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                    ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                        input, &invokestamp_)));
                set_has_invokestamp();
            }
            else {
                goto handle_uninterpreted;
            }
            if (input->ExpectTag(26)) goto parse_args;
            break;
        }

              // repeated .BasicArguments args = 3;
        case 3: {
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
            parse_args:
                DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                    input, add_args()));
            }
            else {
                goto handle_uninterpreted;
            }
            if (input->ExpectTag(26)) goto parse_args;
            if (input->ExpectAtEnd()) return true;
            break;
        }

        default: {
        handle_uninterpreted:
            if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
                ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
                return true;
            }
            DO_(::google::protobuf::internal::WireFormatLite::SkipField(input, tag));
            break;
        }
        }
    }
    return true;
#undef DO_
}

void LargeArgContainer::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream * output) const {
    // required string functionToCall = 1;
    if (has_functiontocall()) {
        ::google::protobuf::internal::WireFormatLite::WriteString(
            1, this->functiontocall(), output);
    }

    // optional int32 invokeStamp = 2;
    if (has_invokestamp()) {
        ::google::protobuf::internal::WireFormatLite::WriteInt32(2, this->invokestamp(), output);
    }

    // repeated .BasicArguments args = 3;
    for (int i = 0; i < this->args_size(); i++) {
        ::google::protobuf::internal::WireFormatLite::WriteMessage(
            3, this->args(i), output);
    }

}

int LargeArgContainer::ByteSize() const {
    int total_size = 0;

    if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
        // required string functionToCall = 1;
        if (has_functiontocall()) {
            total_size += 1 +
                ::google::protobuf::internal::WireFormatLite::StringSize(
                    this->functiontocall());
        }

        // optional int32 invokeStamp = 2;
        if (has_invokestamp()) {
            total_size += 1 +
                ::google::protobuf::internal::WireFormatLite::Int32Size(
                    this->invokestamp());
        }

    }
    // repeated .BasicArguments args = 3;
    total_size += 1 * this->args_size();
    for (int i = 0; i < this->args_size(); i++) {
        total_size +=
            ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
                this->args(i));
    }

    GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
    _cached_size_ = total_size;
    GOOGLE_SAFE_CONCURRENT_WRITES_END();
    return total_size;
}

void LargeArgContainer::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite & from) {
    MergeFrom(*::google::protobuf::down_cast<const LargeArgContainer*>(&from));
}

void LargeArgContainer::MergeFrom(const LargeArgContainer & from) {
    GOOGLE_CHECK_NE(&from, this);
    args_.MergeFrom(from.args_);
    if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
        if (from.has_functiontocall()) {
            set_functiontocall(from.functiontocall());
        }
        if (from.has_invokestamp()) {
            set_invokestamp(from.invokestamp());
        }
    }
}

void LargeArgContainer::CopyFrom(const LargeArgContainer & from) {
    if (&from == this) return;
    Clear();
    MergeFrom(from);
}

bool LargeArgContainer::IsInitialized() const {
    if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

    for (int i = 0; i < args_size(); i++) {
        if (!this->args(i).IsInitialized()) return false;
    }
    return true;
}

void LargeArgContainer::Swap(LargeArgContainer * other) {
    if (other != this) {
        std::swap(functiontocall_, other->functiontocall_);
        std::swap(invokestamp_, other->invokestamp_);
        args_.Swap(&other->args_);
        std::swap(_has_bits_[0], other->_has_bits_[0]);
        std::swap(_cached_size_, other->_cached_size_);
    }
}

::std::string LargeArgContainer::GetTypeName() const {
    return "LargeArgContainer";
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)