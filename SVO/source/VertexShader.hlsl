struct VertOut
{
	float4 pos : SV_POSITION;
};

VertOut vertexMain(float4 position : POSITION)
{
	VertOut o;
	o.pos = position;
	return o;
}