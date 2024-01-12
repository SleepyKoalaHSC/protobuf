// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_generator.h"

#include <sstream>
#include <map>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/compiler/csharp/csharp_options.h"
#include "google/protobuf/compiler/csharp/csharp_reflection_class.h"
#include "google/protobuf/compiler/csharp/names.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

std::map<std::string, int> g_messages; // Keys{ 1:不剔除 2:剔除序列化 3:剔除反序列化 4:整条message剔除 } 

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

Generator::Generator() {}
Generator::~Generator() {}

uint64_t Generator::GetSupportedFeatures() const {
  return CodeGenerator::Feature::FEATURE_PROTO3_OPTIONAL;
}

void GenerateFile(const FileDescriptor* file, io::Printer* printer,
                  const Options* options) {
  ReflectionClassGenerator reflectionClassGenerator(file, options);
  reflectionClassGenerator.Generate(printer);
}

void RecursiveStripping(const Descriptor* message, int strippingType)
{
  for(int i = 0; i < message->field_count(); i++)
  {
    const FieldDescriptor* field = message->field(i);
    if(field->type() == FieldDescriptor::TYPE_MESSAGE)
    {
      const Descriptor* fieldType = field -> message_type();
      std::cout << fieldType->name() << "\n";
      std::string subMessageName = fieldType->name();
      g_messages[subMessageName] = strippingType;
      RecursiveStripping(fieldType, strippingType);
    }
  }
}

void GetStrippingMap(const FileDescriptor* file)
{
  if (file->message_type_count() > 0) {
    std::cout << "message num: " << file->message_type_count() << "\n";
    for (int i = 0; i < file->message_type_count(); i++) {
      const Descriptor* message = file->message_type(i);
      std::string messageName = message->name();
      int strippingType; // 1:不剔除 2:剔除序列化 3:剔除反序列化 4:整条message剔除

      if(messageName.substr(0, 2) == "GC")
        strippingType = 4;
      else if(messageName.substr(0, 2) == "CG")
        strippingType = 3;
      else
        strippingType = 1; // 目前只处理CG和GC

      g_messages[messageName] = strippingType;
    }

    for (int i = 0; i < file->message_type_count(); i++) {
      const Descriptor* message = file->message_type(i);
      std::string messageName = message->name();
      int strippingType = g_messages[messageName]; 

      RecursiveStripping(message, strippingType);
    }
  }

  // for(const auto& message : g_messages) {
  //   const Descriptor* descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(message.first);
  //   std::cout << "name: " << descriptor->name() << ", type: " << message.second << "\n";
  //   RecursiveStripping(descriptor, message.second);
  // }
}

bool Generator::Generate(const FileDescriptor* file,
                         const std::string& parameter,
                         GeneratorContext* generator_context,
                         std::string* error) const {
  std::vector<std::pair<std::string, std::string> > options;
  ParseGeneratorParameter(parameter, &options);

  struct Options cli_options;

  for (size_t i = 0; i < options.size(); i++) {
    if (options[i].first == "file_extension") {
      cli_options.file_extension = options[i].second;
    } else if (options[i].first == "base_namespace") {
      cli_options.base_namespace = options[i].second;
      cli_options.base_namespace_specified = true;
    } else if (options[i].first == "internal_access") {
      cli_options.internal_access = true;
    } else if (options[i].first == "serializable") {
      cli_options.serializable = true;
    } else {
      *error = absl::StrCat("Unknown generator option: ", options[i].first);
      return false;
    }
  }

  std::string filename_error = "";
  std::string filename = GetOutputFile(file,
      cli_options.file_extension,
      cli_options.base_namespace_specified,
      cli_options.base_namespace,
      &filename_error);

  if (filename.empty()) {
    *error = filename_error;
    return false;
  }
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '$');

  GetStrippingMap(file);
  // for (const auto& message : g_messages) {
  //   std::cout << "name: " << message.first << ", type: " << message.second << "\n";
  // }

  GenerateFile(file, &printer, &cli_options);
 
  
  return true;
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
