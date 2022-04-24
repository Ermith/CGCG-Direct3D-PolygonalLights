struct PSIn {
	float4 position : SV_POSITION;
	float4 worldPosition : Position;
	float3 color : Color;
	float3 normal : Normal;
};

cbuffer CBuf {
	float3 viewPos;
};

float4 main(PSIn input) : SV_TARGET
{
	float3 lightPos = float3(-2, 2, 0);
	float3 lightColor = float3(1, 1, 1) * 10;
	float specularHardness = 64.0f;
	input.normal = normalize(input.normal);

	float3 lightDir = lightPos - input.worldPosition.xyz;
	float distance = length(lightDir);
	float distanceSq = distance * distance;
	lightDir = lightDir / distance;
	float3 viewDir = normalize(viewPos - input.worldPosition.xyz);

	float NdotH = dot(input.normal, normalize(lightDir + viewDir));
	float NdotL = dot(input.normal, lightDir);
	float intensityDiff = saturate(NdotL);
	float intensitySpec = pow(saturate(NdotH), specularHardness);
	
	float3 ambient = float3(0.1f, 0.1f, 0.1f);
	float3 specular = intensitySpec * lightColor / distanceSq;
	float3 diffuse = intensityDiff * lightColor / distanceSq;
	return float4(input.color * (ambient + diffuse) + specular, 1);


	/*/
	// Calculate the lighting direction and distance
	float3 lightDir = lightPos - input.worldPosition.xyz;
	float lengthSq = dot(lightDir, lightDir);
	float length = sqrt(lengthSq);
	lightDir /= length;

	// Calculate the view and reflection/halfway direction
	float3 viewDir = normalize(viewPos - input.worldPosition.xyz);
	// Cheaper approximation of reflected direction = reflect(-lightDir, normal)
	//float halfDir = -lightDir - dot(-lightDir, input.normal);
	float3 halfDir = normalize(reflect(-lightDir, input.normal));
	//float3 halfDir = normalize(viewDir + lightDir);

	// Calculate diffuse and specular coefficients
	float NdotL = max(0.0f, dot(input.normal, lightDir));
	float NdotH = max(0.0f, dot(viewDir, halfDir));

	// Calculate the Phong model terms: ambient, diffuse, specular
	float3 ambient = float3(0.25f, 0.25f, 0.25f);
	float3 diffuse = NdotL * lightColor;
	float3 specular = lightColor * pow(NdotH, 32.0f) / lengthSq;

	return float4(input.color * (ambient + diffuse) + specular, 1);
	//*/
}