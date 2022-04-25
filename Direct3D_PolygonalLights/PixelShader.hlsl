static const int LightBufferSize = 10;

struct PointLight {
	float4 Position;
	float4 Color;
};

struct DirLight {
	float4 Direction;
	float4 Color;
};

struct SpotLight {
	float4 Position;
	float4 Direction;
	float4 Color;
	float4 InnerCone;
	float4 OuterCone;
};

struct PSIn {
	float4 position : SV_POSITION;
	float4 worldPosition : Position;
	float3 color : Color;
	float3 normal : Normal;
};

float4 CalcDirLight(DirLight light, float3 normal, float4 fragColor, float3 viewDir, float shadow = 0.0, float4 specularity = 1.0, float exponent = 64) {
	float3 lightDir = light.Direction.xyz;

	float NdotH = dot(normal, normalize(lightDir + viewDir));
	float NdotL = dot(lightDir, normalize(normal));
	float intensityDiff = saturate(NdotL);
	float intensitySpec = pow(saturate(NdotH), exponent);

	float3 ambient = float3(1, 1, 1) * 0.2;
	float3 specular = intensitySpec * light.Color * 10;
	float3 diffuse = intensityDiff * light.Color;
//	return float4(diffuse * fragColor, 1);
	return float4(fragColor * (ambient + diffuse) + specular, 1);
}

float4 CalcPointLight(
	PointLight light,
	float3 normal,
	float3 fragPos,
	float4 fragColor,
	float3 viewDir,
	float4 specularity = 1.0,
	float exponent = 64,
	float ambientStr = 0.25
) {
	float3 lightDir = light.Position.xyz - fragPos;
	float distance = length(lightDir);
	float distanceSq = distance * distance;
	lightDir = lightDir / distance;

	float NdotH = dot(normal, normalize(lightDir + viewDir));
	float NdotL = dot(lightDir, normal);
	float intensityDiff = saturate(NdotL);
	float intensitySpec = pow(saturate(NdotH), exponent);

	float3 ambient = float3(1, 1, 1) * ambientStr;
	float3 specular = intensitySpec * light.Color / distanceSq * 20;
	float3 diffuse = intensityDiff * light.Color / distanceSq * 20;
	return float4(fragColor * (ambient + diffuse) + specular, 1);
}

float4 CalcSpotLight(SpotLight light, float3 normal, float3 fragPos, float4 fragColor, float3 viewDir, float4 specularity = 1.0, float exponent = 32, float ambientStr = 0.2) {
	float3 lightDir = light.Position.xyz - fragPos;
	float distance = length(lightDir);
	float distanceSq = distance * distance;
	lightDir = lightDir / distance;

	float NdotH = dot(normal, normalize(lightDir + viewDir));
	float NdotL = dot(lightDir, normal);
	float intensityDiff = saturate(NdotL);
	float intensitySpec = pow(saturate(NdotH), exponent);

	float3 ambient = float3(1, 1, 1) * ambientStr;
	float3 specular = intensitySpec * light.Color / distanceSq * 50;
	float3 diffuse = intensityDiff * light.Color / distanceSq * 50;

	float theta = dot(lightDir, normalize(-light.Direction));
	float epsilon = light.InnerCone.x - light.OuterCone.x;
	float intensity = clamp((theta - light.OuterCone.x) / epsilon, 0.0, 1.0);

	ambient *= intensity;
	diffuse *= intensity;
	specular *= intensity;

	float4 finalColor = saturate(float4((ambient + diffuse + specular), 1) * fragColor);
	finalColor.a = 1;

	return finalColor;
}

cbuffer CBuf {
	float4 viewPos;
	int4 lightCounts;
	PointLight pointLights[LightBufferSize];
	SpotLight spotLights[LightBufferSize];
	DirLight dirLights[LightBufferSize];
};

float4 main(PSIn input) : SV_TARGET {
	float3 viewDir = normalize(viewPos - input.worldPosition.xyz);

	float4 finalLight = float4(0, 0, 0, 0);
	for (int i = 0; i < lightCounts.x; i++)
		finalLight += CalcPointLight(pointLights[i], input.normal, input.worldPosition.xyz, float4(input.color, 1), viewDir);

	for (int i = 0; i < lightCounts.y; i++)
		finalLight += CalcSpotLight(spotLights[i], input.normal, input.worldPosition.xyz, float4(input.color, 1), viewDir);

	for (int i = 0; i < lightCounts.z; i++)
		finalLight += CalcDirLight(dirLights[i], input.normal, float4(input.color, 1), viewDir);

	return finalLight;
}