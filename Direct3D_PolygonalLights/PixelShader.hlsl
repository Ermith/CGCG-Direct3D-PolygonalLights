struct PSIn {
	float4 position : SV_POSITION;
	float3 color : Color;
};

float4 main(PSIn input) : SV_TARGET
{
	return float4(input.color, 1);
}