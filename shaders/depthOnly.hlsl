
cbuffer ConstantBufferCamera : register( b0 )
{
	matrix worldViewProj;
	matrix world;
	float4 eye;
}


struct VertexShaderOutput
{
    float4 position : SV_POSITION;
};

VertexShaderOutput vertexShaderMain( float4 position :	position,
									 float4 normal :	normal,
									 float4 color :		color,
									 float4 texCoord :	texCoord )
{
    VertexShaderOutput output;
    output.position = mul( position, worldViewProj );
    return output;
}

float4 pixelShaderMain( VertexShaderOutput input ) : SV_Target
{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
