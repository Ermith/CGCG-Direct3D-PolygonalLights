struct PSIn {
	float4 position : SV_POSITION;
	float4 worldPosition : Position;
	float3 color : Color;
};

cbuffer CBuf {
	float3 viewPos;
};

float4 main(PSIn input) : SV_TARGET
{
	float3 lightPos = float3(0, 4, 0);
	float3 lightColor = float3(15, 15, 15);

	// Calculate the lighting direction and distance
	float3 lightDir = lightPos - input.worldPosition.xyz;
	float lengthSq = dot(lightDir, lightDir);
	float length = sqrt(lengthSq);
	lightDir /= length;


	// Calculate the view and reflection/halfway direction
	float3 viewDir = normalize(viewPos - input.worldPosition.xyz);
	// Cheaper approximation of reflected direction = reflect(-lightDir, normal)
	float3 halfDir = normalize(viewDir + lightDir);

	// Calculate diffuse and specular coefficients
	float NdotL = max(0.0f, dot(float3(0,1,0), lightDir));
	float NdotH = max(0.0f, dot(float3(0,1,0), halfDir));

	// Calculate the Phong model terms: ambient, diffuse, specular
	float3 ambient = float3(0.01f, 0.01f, 0.01f);
	float3 diffuse = NdotL * lightColor / lengthSq;
	float3 specular = lightColor * pow(NdotH, 64.0f) / lengthSq;

	return float4(input.color * (ambient + diffuse) + specular, 1);
}