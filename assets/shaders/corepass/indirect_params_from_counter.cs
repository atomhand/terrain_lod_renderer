#version 460
#inject

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, std430) readonly buffer counterSsbo {
    uint counter[];
};

struct DispatchIndirectCommand {
    uint num_groups_x;
    uint num_groups_y;
    uint num_groups_z;
};

layout(binding = 1, std430) writeonly buffer drawCommandsSsbo {
    DispatchIndirectCommand params[];
};

void main() {
    params[0] = DispatchIndirectCommand((counter[0]+255)/256, 1, 1);
}