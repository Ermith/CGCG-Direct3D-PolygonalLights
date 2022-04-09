struct VSOut {
	float4 position : SV_POSITION;
	float3 color : Color;
};

VSOut main(float3 pos : Position, float3 col : Color)
{
	VSOut o;
	o.position = float4(pos, 1);
	o.color = col;
	return o;
}