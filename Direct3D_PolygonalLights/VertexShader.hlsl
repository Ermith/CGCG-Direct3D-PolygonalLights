struct VSOut {
	float4 position : SV_POSITION;
	float4 worldPosition : Position;
	float3 color : Color;
	float3 normal : Normal;
};

cbuffer CBuf {
	matrix modelToWorld;
	matrix worldToView;
	matrix projection;
	matrix normalTransform;
};

VSOut main(float3 pos : Position, float3 col : Color, float3 normal : Normal)
{
	VSOut o;
	o.worldPosition = mul(float4(pos, 1), modelToWorld);
	o.position = mul(o.worldPosition, worldToView);
	o.position = mul(o.position, projection);
	o.normal = mul(float4(normal, 1), normalTransform).xyz;
	o.normal = normal;
	o.color = col;
	return o;
}