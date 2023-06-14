/*
* Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
*  http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

// NOTE: This file is generated and may not follow lint rules defined in your app
// Generated files can be excluded from analysis in analysis_options.yaml
// For more info, see: https://dart.dev/guides/language/analysis-options#excluding-code-from-analysis

// ignore_for_file: public_member_api_docs, annotate_overrides, dead_code, dead_codepublic_member_api_docs, depend_on_referenced_packages, file_names, library_private_types_in_public_api, no_leading_underscores_for_library_prefixes, no_leading_underscores_for_local_identifiers, non_constant_identifier_names, null_check_on_nullable_type_parameter, prefer_adjacent_string_concatenation, prefer_const_constructors, prefer_if_null_operators, prefer_interpolation_to_compose_strings, slash_for_doc_comments, sort_child_properties_last, unnecessary_const, unnecessary_constructor_name, unnecessary_late, unnecessary_new, unnecessary_null_aware_assignments, unnecessary_nullable_for_final_variable_declarations, unnecessary_string_interpolations, use_build_context_synchronously

import 'package:amplify_core/amplify_core.dart';
import 'package:flutter/foundation.dart';


/** This is an auto generated class representing the DataDriven type in your schema. */
@immutable
class DataDriven extends Model {
  static const classType = const _DataDrivenModelType();
  final String id;
  final String? _Username;
  final String? _CarModel;
  final String? _Location;
  final int? _Temperature;
  final int? _EngRev;
  final int? _Speed;
  final TemporalDateTime? _createdAt;
  final TemporalDateTime? _updatedAt;

  @override
  getInstanceType() => classType;
  
  @override
  String getId() {
    return id;
  }
  
  String? get Username {
    return _Username;
  }
  
  String? get CarModel {
    return _CarModel;
  }
  
  String? get Location {
    return _Location;
  }
  
  int? get Temperature {
    return _Temperature;
  }
  
  int? get EngRev {
    return _EngRev;
  }
  
  int? get Speed {
    return _Speed;
  }
  
  TemporalDateTime? get createdAt {
    return _createdAt;
  }
  
  TemporalDateTime? get updatedAt {
    return _updatedAt;
  }
  
  const DataDriven._internal({required this.id, Username, CarModel, Location, Temperature, EngRev, Speed, createdAt, updatedAt}): _Username = Username, _CarModel = CarModel, _Location = Location, _Temperature = Temperature, _EngRev = EngRev, _Speed = Speed, _createdAt = createdAt, _updatedAt = updatedAt;
  
  factory DataDriven({String? id, String? Username, String? CarModel, String? Location, int? Temperature, int? EngRev, int? Speed}) {
    return DataDriven._internal(
      id: id == null ? UUID.getUUID() : id,
      Username: Username,
      CarModel: CarModel,
      Location: Location,
      Temperature: Temperature,
      EngRev: EngRev,
      Speed: Speed);
  }
  
  bool equals(Object other) {
    return this == other;
  }
  
  @override
  bool operator ==(Object other) {
    if (identical(other, this)) return true;
    return other is DataDriven &&
      id == other.id &&
      _Username == other._Username &&
      _CarModel == other._CarModel &&
      _Location == other._Location &&
      _Temperature == other._Temperature &&
      _EngRev == other._EngRev &&
      _Speed == other._Speed;
  }
  
  @override
  int get hashCode => toString().hashCode;
  
  @override
  String toString() {
    var buffer = new StringBuffer();
    
    buffer.write("DataDriven {");
    buffer.write("id=" + "$id" + ", ");
    buffer.write("Username=" + "$_Username" + ", ");
    buffer.write("CarModel=" + "$_CarModel" + ", ");
    buffer.write("Location=" + "$_Location" + ", ");
    buffer.write("Temperature=" + (_Temperature != null ? _Temperature!.toString() : "null") + ", ");
    buffer.write("EngRev=" + (_EngRev != null ? _EngRev!.toString() : "null") + ", ");
    buffer.write("Speed=" + (_Speed != null ? _Speed!.toString() : "null") + ", ");
    buffer.write("createdAt=" + (_createdAt != null ? _createdAt!.format() : "null") + ", ");
    buffer.write("updatedAt=" + (_updatedAt != null ? _updatedAt!.format() : "null"));
    buffer.write("}");
    
    return buffer.toString();
  }
  
  DataDriven copyWith({String? id, String? Username, String? CarModel, String? Location, int? Temperature, int? EngRev, int? Speed}) {
    return DataDriven._internal(
      id: id ?? this.id,
      Username: Username ?? this.Username,
      CarModel: CarModel ?? this.CarModel,
      Location: Location ?? this.Location,
      Temperature: Temperature ?? this.Temperature,
      EngRev: EngRev ?? this.EngRev,
      Speed: Speed ?? this.Speed);
  }
  
  DataDriven.fromJson(Map<String, dynamic> json)  
    : id = json['id'],
      _Username = json['Username'],
      _CarModel = json['CarModel'],
      _Location = json['Location'],
      _Temperature = (json['Temperature'] as num?)?.toInt(),
      _EngRev = (json['EngRev'] as num?)?.toInt(),
      _Speed = (json['Speed'] as num?)?.toInt(),
      _createdAt = json['createdAt'] != null ? TemporalDateTime.fromString(json['createdAt']) : null,
      _updatedAt = json['updatedAt'] != null ? TemporalDateTime.fromString(json['updatedAt']) : null;
  
  Map<String, dynamic> toJson() => {
    'id': id, 'Username': _Username, 'CarModel': _CarModel, 'Location': _Location, 'Temperature': _Temperature, 'EngRev': _EngRev, 'Speed': _Speed, 'createdAt': _createdAt?.format(), 'updatedAt': _updatedAt?.format()
  };
  
  Map<String, Object?> toMap() => {
    'id': id, 'Username': _Username, 'CarModel': _CarModel, 'Location': _Location, 'Temperature': _Temperature, 'EngRev': _EngRev, 'Speed': _Speed, 'createdAt': _createdAt, 'updatedAt': _updatedAt
  };

  static final QueryField ID = QueryField(fieldName: "id");
  static final QueryField USERNAME = QueryField(fieldName: "Username");
  static final QueryField CARMODEL = QueryField(fieldName: "CarModel");
  static final QueryField LOCATION = QueryField(fieldName: "Location");
  static final QueryField TEMPERATURE = QueryField(fieldName: "Temperature");
  static final QueryField ENGREV = QueryField(fieldName: "EngRev");
  static final QueryField SPEED = QueryField(fieldName: "Speed");
  static var schema = Model.defineSchema(define: (ModelSchemaDefinition modelSchemaDefinition) {
    modelSchemaDefinition.name = "DataDriven";
    modelSchemaDefinition.pluralName = "DataDrivens";
    
    modelSchemaDefinition.authRules = [
      AuthRule(
        authStrategy: AuthStrategy.PUBLIC,
        operations: [
          ModelOperation.CREATE,
          ModelOperation.UPDATE,
          ModelOperation.DELETE,
          ModelOperation.READ
        ])
    ];
    
    modelSchemaDefinition.addField(ModelFieldDefinition.id());
    
    modelSchemaDefinition.addField(ModelFieldDefinition.field(
      key: DataDriven.USERNAME,
      isRequired: false,
      ofType: ModelFieldType(ModelFieldTypeEnum.string)
    ));
    
    modelSchemaDefinition.addField(ModelFieldDefinition.field(
      key: DataDriven.CARMODEL,
      isRequired: false,
      ofType: ModelFieldType(ModelFieldTypeEnum.string)
    ));
    
    modelSchemaDefinition.addField(ModelFieldDefinition.field(
      key: DataDriven.LOCATION,
      isRequired: false,
      ofType: ModelFieldType(ModelFieldTypeEnum.string)
    ));
    
    modelSchemaDefinition.addField(ModelFieldDefinition.field(
      key: DataDriven.TEMPERATURE,
      isRequired: false,
      ofType: ModelFieldType(ModelFieldTypeEnum.int)
    ));
    
    modelSchemaDefinition.addField(ModelFieldDefinition.field(
      key: DataDriven.ENGREV,
      isRequired: false,
      ofType: ModelFieldType(ModelFieldTypeEnum.int)
    ));
    
    modelSchemaDefinition.addField(ModelFieldDefinition.field(
      key: DataDriven.SPEED,
      isRequired: false,
      ofType: ModelFieldType(ModelFieldTypeEnum.int)
    ));
    
    modelSchemaDefinition.addField(ModelFieldDefinition.nonQueryField(
      fieldName: 'createdAt',
      isRequired: false,
      isReadOnly: true,
      ofType: ModelFieldType(ModelFieldTypeEnum.dateTime)
    ));
    
    modelSchemaDefinition.addField(ModelFieldDefinition.nonQueryField(
      fieldName: 'updatedAt',
      isRequired: false,
      isReadOnly: true,
      ofType: ModelFieldType(ModelFieldTypeEnum.dateTime)
    ));
  });
}

class _DataDrivenModelType extends ModelType<DataDriven> {
  const _DataDrivenModelType();
  
  @override
  DataDriven fromJson(Map<String, dynamic> jsonData) {
    return DataDriven.fromJson(jsonData);
  }
}