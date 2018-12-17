
cbuffer ConstantBufferCamera : register( b0 )
{
	matrix worldViewProj;
	matrix world;
	float4 eye;
}

cbuffer ConstantBufferMaterial : register( b1 )
{
	float4	ambient;
	float4	diffuse;
	float4	specular;
	float	shiny;
	float3	dummyMaterial;
}

cbuffer ConstantBufferLight : register ( b2 )
{
	float4 lightDir;
}



struct VertexShaderOutput
{
    float4 position : SV_POSITION;
	float4 light : TEXCOORD0;
	float4 normal : TEXCOORD1;
	float4 view : TEXCOORD2;
};

VertexShaderOutput vertexShaderMain( float4 position :	position,
									 float4 normal :	normal,
									 float4 color :		color,
									 float4 texCoord :	texCoord )
{
    VertexShaderOutput output;
    output.position = mul( position, worldViewProj );
	output.light = lightDir;
	float4 posWorld = normalize(mul(position, world)); 
	output.view = eye - posWorld;
	normal.w = 0.0f;
	output.normal = mul(normal, world);
    return output;
}

float4 pixelShaderMain( VertexShaderOutput input ) : SV_Target
{

	float4 normal = normalize(input.normal);
	float4 lightDir = normalize(input.light);
	float4 viewDir = normalize(input.view);
	//float4 diff = saturate(dot(normal, lightDir)); // diffuse component
	float4 diff = abs(dot(normal, lightDir)); // diffuse component

	// R = 2 * (N.L) * N - L
	float4 reflect = normalize(2 * diff * normal - lightDir);
	float4 specular = 0.0f;
	if (shiny > 0.0f) {
		specular = pow(saturate(dot(reflect, viewDir)), shiny); // R.V^n
	}

	// I = Acolor + Dcolor * N.L + (R.V)n
	return ambient + diffuse * diff + specular; 
}
