struct VSIn {
	float4 position : SV_POSITION;
	float3 color : Color;
};

float4 main(VSIn input) : SV_TARGET
{
	return float4(input.color, 1);
}