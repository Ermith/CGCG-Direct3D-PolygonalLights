static const int LightBufferSize = 10;
static const float pi = 3.14159265;
static const float LUT_SIZE = 64.0;
static const float LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
static const float LUT_BIAS = 0.5 / LUT_SIZE;

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
	float4 Cone;
};

struct RectLight {
	float4 Position;
	float4 Params; // Width, Height, RotY, RotZ
	float4 Color;
};

struct PSIn {
	float4 position : SV_POSITION;
	float4 worldPosition : Position;
	float3 color : Color;
	float2 uv : Texture;
	float3 normal : Normal;
	unsigned int lightIndex : Index;
};

Texture2D ltcMat;
Texture2D ltcAmp;
Texture2D lightTexture;
SamplerState ltcSampler;

cbuffer CBuf {
	float4 viewPos;
	int4 lightCounts; // point, spot, dir, rect
	PointLight pointLights[LightBufferSize];
	SpotLight spotLights[LightBufferSize];
	DirLight dirLights[LightBufferSize];
	RectLight rectLights[LightBufferSize];
};

float4 CalcDirLight(DirLight light, float3 normal, float4 fragColor, float3 viewDir, float shadow = 0.0, float4 specularity = 1.0, float exponent = 64) {
	float3 lightDir = light.Direction.xyz;
	float intensity = light.Color.w;

	float NdotH = dot(normal, normalize(lightDir + viewDir));
	float NdotL = dot(lightDir, normalize(normal));
	float intensityDiff = saturate(NdotL);
	float intensitySpec = pow(saturate(NdotH), exponent);

	float3 ambient = float3(1, 1, 1) * 0.2;
	float3 specular = intensitySpec * light.Color.xyz * 10;
	float3 diffuse = intensityDiff * light.Color.xyz;
//	return float4(diffuse * fragColor, 1);
	return float4(fragColor.xyz * (ambient + diffuse) + specular, 1) * intensity;
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
	float3 specular = intensitySpec * light.Color.xyz / distanceSq * 20;
	float3 diffuse = intensityDiff * light.Color.xyz / distanceSq * 20;
	//return float4(diffuse * fragColor, 1);
	return float4(fragColor.xyz * (ambient + diffuse) + specular, 1);
}

float4 CalcSpotLight(SpotLight light, float3 normal, float3 fragPos, float4 fragColor, float3 viewDir, float4 specularity = 1.0, float exponent = 32, float ambientStr = 0.2) {
	float innerCone = light.Cone.x;
	float outerCone = light.Cone.y;

	float3 lightDir = light.Position.xyz - fragPos;
	float distance = length(lightDir);
	float distanceSq = distance * distance;
	lightDir = lightDir / distance;

	float NdotH = dot(normal, normalize(lightDir + viewDir));
	float NdotL = dot(lightDir, normal);
	float intensityDiff = saturate(NdotL);
	float intensitySpec = pow(saturate(NdotH), exponent);

	float3 ambient = float3(1, 1, 1) * ambientStr;
	float3 specular = intensitySpec * light.Color.xyz / distanceSq * 50;
	float3 diffuse = intensityDiff * light.Color.xyz / distanceSq * 50;

	float theta = dot(lightDir, normalize(-light.Direction.xyz));
	float epsilon = innerCone.x - outerCone.x;
	float intensity = clamp((theta - outerCone.x) / epsilon, 0.0, 1.0);

	ambient *= intensity;
	diffuse *= intensity;
	specular *= intensity;

	float4 finalColor = saturate(float4((ambient + diffuse + specular), 1) * fragColor);
	finalColor.a = 1;

	return finalColor;
}

float3 rotation_y(float3 v, float a)
{
	float3 r;
	r.x = v.x * cos(a) + v.z * sin(a);
	r.y = v.y;
	r.z = -v.x * sin(a) + v.z * cos(a);
	return r;
}

float3 rotation_z(float3 v, float a)
{
	float3 r;
	r.x = v.x * cos(a) - v.y * sin(a);
	r.y = v.x * sin(a) + v.y * cos(a);
	r.z = v.z;
	return r;
}

float3 rotation_yz(float3 v, float ay, float az)
{
	return rotation_z(rotation_y(v, ay), az);
}


float IntegrateEdge(float3 v1, float3 v2) {
	float cosTheta = dot(v1, v2);
	float theta = acos(cosTheta);
	float res = cross(v1, v2).z * ((theta > 0.001) ? theta / sin(theta) : 1.0);

	return res;
}

void ClipQuadToHorizon(inout float3 L[5], out int n)
{
	// detect clipping config
	int config = 0;
	if (L[0].z > 0.0) config += 1;
	if (L[1].z > 0.0) config += 2;
	if (L[2].z > 0.0) config += 4;
	if (L[3].z > 0.0) config += 8;

	// clip
	n = 0;

	if (config == 0)
	{
		// clip all
	}
	else if (config == 1) // V1 clip V2 V3 V4
	{
		n = 3;
		L[1] = -L[1].z * L[0] + L[0].z * L[1];
		L[2] = -L[3].z * L[0] + L[0].z * L[3];
	}
	else if (config == 2) // V2 clip V1 V3 V4
	{
		n = 3;
		L[0] = -L[0].z * L[1] + L[1].z * L[0];
		L[2] = -L[2].z * L[1] + L[1].z * L[2];
	}
	else if (config == 3) // V1 V2 clip V3 V4
	{
		n = 4;
		L[2] = -L[2].z * L[1] + L[1].z * L[2];
		L[3] = -L[3].z * L[0] + L[0].z * L[3];
	}
	else if (config == 4) // V3 clip V1 V2 V4
	{
		n = 3;
		L[0] = -L[3].z * L[2] + L[2].z * L[3];
		L[1] = -L[1].z * L[2] + L[2].z * L[1];
	}
	else if (config == 5) // V1 V3 clip V2 V4) impossible
	{
		n = 0;
	}
	else if (config == 6) // V2 V3 clip V1 V4
	{
		n = 4;
		L[0] = -L[0].z * L[1] + L[1].z * L[0];
		L[3] = -L[3].z * L[2] + L[2].z * L[3];
	}
	else if (config == 7) // V1 V2 V3 clip V4
	{
		n = 5;
		L[4] = -L[3].z * L[0] + L[0].z * L[3];
		L[3] = -L[3].z * L[2] + L[2].z * L[3];
	}
	else if (config == 8) // V4 clip V1 V2 V3
	{
		n = 3;
		L[0] = -L[0].z * L[3] + L[3].z * L[0];
		L[1] = -L[2].z * L[3] + L[3].z * L[2];
		L[2] = L[3];
	}
	else if (config == 9) // V1 V4 clip V2 V3
	{
		n = 4;
		L[1] = -L[1].z * L[0] + L[0].z * L[1];
		L[2] = -L[2].z * L[3] + L[3].z * L[2];
	}
	else if (config == 10) // V2 V4 clip V1 V3) impossible
	{
		n = 0;
	}
	else if (config == 11) // V1 V2 V4 clip V3
	{
		n = 5;
		L[4] = L[3];
		L[3] = -L[2].z * L[3] + L[3].z * L[2];
		L[2] = -L[2].z * L[1] + L[1].z * L[2];
	}
	else if (config == 12) // V3 V4 clip V1 V2
	{
		n = 4;
		L[1] = -L[1].z * L[2] + L[2].z * L[1];
		L[0] = -L[0].z * L[3] + L[3].z * L[0];
	}
	else if (config == 13) // V1 V3 V4 clip V2
	{
		n = 5;
		L[4] = L[3];
		L[3] = L[2];
		L[2] = -L[1].z * L[2] + L[2].z * L[1];
		L[1] = -L[1].z * L[0] + L[0].z * L[1];
	}
	else if (config == 14) // V2 V3 V4 clip V1
	{
		n = 5;
		L[4] = -L[0].z * L[3] + L[3].z * L[0];
		L[0] = -L[0].z * L[1] + L[1].z * L[0];
	}
	else if (config == 15) // V1 V2 V3 V4
	{
		n = 4;
	}

	if (n == 3)
		L[3] = L[0];
	if (n == 4)
		L[4] = L[0];
}

float3 FetchDiffuseFilteredTexture(float3 p1_, float3 p2_, float3 p3_, float3 p4_)
{
	// area light plane basis
	float3 V1 = p2_ - p1_;
	float3 V2 = p4_ - p1_;
	float3 planeOrtho = (cross(V1, V2));
	float planeAreaSquared = dot(planeOrtho, planeOrtho);
	float planeDistxPlaneArea = dot(planeOrtho, p1_);
	// orthonormal projection of (0,0,0) in area light space
	float3 P = planeDistxPlaneArea * planeOrtho / planeAreaSquared - p1_;

	// find tex coords of P
	float dot_V1_V2 = dot(V1, V2);
	float inv_dot_V1_V1 = 1.0 / dot(V1, V1);
	float3 V2_ = V2 - V1 * dot_V1_V2 * inv_dot_V1_V1;
	float2 Puv;
	Puv.y = dot(V2_, P) / dot(V2_, V2_);
	Puv.x = dot(V1, P) * inv_dot_V1_V1 - dot_V1_V2 * inv_dot_V1_V1 * Puv.y;

	// LOD
	float d = abs(planeDistxPlaneArea) / pow(planeAreaSquared, 0.75);
	//Puv.y *= Puv.y * Puv.y * Puv.y * Puv.y;
	//return lightTexture.Sample(ltcSampler, float2(0.125, 0.125) + 0.75 * Puv);
	return lightTexture.SampleLevel(ltcSampler, float2(0.125, 0.125) + 0.75 * Puv, log(2048.0 * d) / log(3.0)).xyz;
}

float4 LTCEvaluate(RectLight light, float3 fragPos, float3 viewDir, float3 normal, float3 points[4], float3x3 Minv, out float4 color) {
	float3 T1, T2;
	T1 = normalize(viewDir - normal * dot(viewDir, normal));
	T2 = cross(normal, T1);

	float3x3 M;
	M[0] = T1;
	M[1] = T2;
	M[2] = normal;
	Minv = mul(transpose(M), Minv);

	float3 L[5];
	L[0] = mul(points[0] - fragPos, Minv);
	L[1] = mul(points[1] - fragPos, Minv);
	L[2] = mul(points[2] - fragPos, Minv);
	L[3] = mul(points[3] - fragPos, Minv);

	float3 texturedCol = FetchDiffuseFilteredTexture(L[0], L[1], L[2], L[3]);

	int n = 0;
	ClipQuadToHorizon(L, n);

	if (n == 0)
		return float4(0, 0, 0, 0);

	// project onto sphere
	L[0] = normalize(L[0]);
	L[1] = normalize(L[1]);
	L[2] = normalize(L[2]);
	L[3] = normalize(L[3]);
	L[4] = normalize(L[4]);

	float sum = 0;
	sum += IntegrateEdge(L[0], L[1]);
	sum += IntegrateEdge(L[1], L[2]);
	sum += IntegrateEdge(L[2], L[3]);
	if (n >= 4)
		sum += IntegrateEdge(L[3], L[4]);
	if (n == 5)
		sum += IntegrateEdge(L[4], L[0]);

	sum = abs(sum);
	color = float4(texturedCol, 1);
	return float4(sum, sum, sum, 1) * color;
}

float4 CalcRectLight(
	RectLight light,
	float3 normal,
	float3 fragPos,
	float4 fragColor,
	float3 viewDir,
	float roughness = 0.25
	)
{
	float3 lightPos = light.Position.xyz;
	float lightIntensity = light.Color.w;
	float halfWidth = light.Params[0];
	float halfHeight = light.Params[1];
	float rotateY = light.Params[2]*2*pi;
	float rotateZ = light.Params[3]*2*pi;

	float3 dirX = rotation_yz(float3(1, 0, 0), rotateY, rotateZ);
	float3 dirY = rotation_yz(float3(0, 1, 0), rotateY, rotateZ);

	float3 ex = halfWidth * dirX;
	float3 ey = halfHeight * dirY;

	float3 points[4];
	points[0] = lightPos - ex - ey;
	points[1] = lightPos + ex - ey;
	points[2] = lightPos + ex + ey;
	points[3] = lightPos - ex + ey;

	float4 diffuseColor = float4(0, 0, 0, 0);
	float4 specularColor = float4(0, 0, 0, 0);

	float3x3 identity = float3x3(
		1, 0, 0,
		0, 1, 0,
		0, 0, 1);
	float4 diffuse = LTCEvaluate(light, fragPos, viewDir, normal, points, identity, diffuseColor);
	diffuse *= 1.5;

	// MAKE MATRIX SAMPLE
	float theta = acos(dot(normal, viewDir));
	float2 uv = float2(roughness, theta/(0.5*pi));
	uv = uv * LUT_SCALE + LUT_BIAS;

	float4 t = ltcMat.Sample(ltcSampler, uv);
	float3x3 Minv = float3x3(
		1, 0, t.y,
		0, t.z, 0,
		t.w, 0, t.x
		);
	float4 specular = LTCEvaluate(light, fragPos, viewDir, normal, points, Minv, specularColor);
	specular *= ltcAmp.Sample(ltcSampler, uv).w * 0.2;

	float4 ambient = float4(0.05, 0.05, 0.05, 1);

	//diffuseColor = float4(1, 1, 1, 1);
	//specularColor = float4(1, 1, 1, 1);
	//float4 lightColor = light.Color;
	//float4 diffuseColor = chainTexture.Sample(ltcSampler, uv);
	float4 col = (specularColor * specular + diffuse * diffuseColor) *light.Color;
	col *= lightIntensity;
	col /= 2.0 * pi;
	col += ambient * fragColor;
	col.w = 1;
	return col;
}

float4 main(PSIn input) : SV_TARGET{

	if (input.lightIndex <= (unsigned int)lightCounts.w)
		return lightTexture.SampleLevel(ltcSampler, float2(0.125, 0.125) + input.uv * 0.75, 1);

	float3 viewDir = normalize(viewPos.xyz - input.worldPosition.xyz);

	float4 finalLight = float4(0, 0, 0, 1);
	for (int i = 0; i < lightCounts.x; i++)
		finalLight += CalcPointLight(pointLights[i], input.normal, input.worldPosition.xyz, float4(input.color, 1), viewDir);

	for (i = 0; i < lightCounts.y; i++)
		finalLight += CalcSpotLight(spotLights[i], input.normal, input.worldPosition.xyz, float4(input.color, 1), viewDir);

	for (i = 0; i < lightCounts.z; i++)
		finalLight += CalcDirLight(dirLights[i], input.normal, float4(input.color, 1), viewDir);

	for (i = 0; i < lightCounts.w; i++)
		finalLight += CalcRectLight(rectLights[i], input.normal, input.worldPosition.xyz, float4(input.color, 1), viewDir);

	return finalLight;
}