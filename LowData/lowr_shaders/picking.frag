#version 450

layout(location = 0) flat in uint in_PickId;
layout(location = 0) out uint o_ObjectId;

void main() {
   o_ObjectId = in_PickId; 
}
