struct VSOut {
	float4 position : SV_POSITION;
	float3 color : Color;
};

cbuffer CBuf {
	matrix transform;
};

VSOut main(float3 pos : Position, float3 col : Color)
{
	VSOut o;
	o.position = mul(float4(pos, 1), transform);
	o.color = col;
	return o;
}