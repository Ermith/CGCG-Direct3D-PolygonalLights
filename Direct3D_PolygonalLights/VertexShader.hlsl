static const int MAX_OBJECTS = 100;

struct VSOut {
	float4 position : SV_POSITION;
	float4 worldPosition : Position;
	float3 color : Color;
	float3 normal : Normal;
	unsigned int lightIndex : Index;
};

cbuffer CBuf {
	matrix modelToWorld[MAX_OBJECTS];
	matrix worldToView;
	matrix projection;
	matrix normalTransform[MAX_OBJECTS];
	unsigned int objects;
};

VSOut main(float3 pos : Position, float3 col : Color, float3 normal : Normal, unsigned int index : Index)
{
	VSOut o;
	o.worldPosition = mul(float4(pos, 1), modelToWorld[index]);
	o.position = mul(o.worldPosition, worldToView);
	o.position = mul(o.position, projection);
	o.normal = mul(float4(normal, 1), normalTransform[index]).xyz;
	//o.normal = normal;
	o.color = col;
	o.lightIndex = objects - index;
	return o;
}