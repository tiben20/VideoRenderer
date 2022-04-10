

Texture2D tex : register(t0);
Texture2D featureMap11 : register(t0);
Texture2D featureMap22 : register(t1);
Texture2D tex1 : register(t2);
Texture2D tex2 : register(t3);
Texture2D tex3 : register(t4);
Texture2D tex4 : register(t5);
SamplerState samp : register(s0);

Texture2D yuvTex;

cbuffer PS_CONSTANTS : register(b0)
{
    float4 size0;
    float inputPtY;
    float inputPtX;
    float2 fArgs;
    int4 iArgs;
    float2 int_argsf;
    int2 int_argsi;
};

const static float3x3 _rgb2yuv =
{
    0.299, 0.587, 0.114,
	-0.169, -0.331, 0.5,
	0.5, -0.419, -0.081
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 target1 : SV_Target0;
    float4 target2 : SV_Target1;
};
const static float3x3 _yuv2rgb =
{
    1, -0.00093, 1.401687,
	1, -0.3437, -0.71417,
	1, 1.77216, 0.00099
};

PS_OUTPUT main(PS_INPUT input) : SV_Target
{
    PS_OUTPUT outp;

    outp.target1 = float4(-0.1572492271661758, -0.0120896836742759, 0.0061487639322877, -0.2852848768234253);
    outp.target1 += float4(-0.0047900392673910, 0.0537447109818459, -0.0000247144635068, 0.0066653941757977)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, -2 * inputPtY)).x;
    outp.target1 += float4(0.0073144687339664, -0.0309004038572311, -0.0109181385487318, -0.0092840325087309)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, -inputPtY)).x;
    outp.target1 += float4(0.0591700896620750, 0.1974907070398331, -0.0197357516735792, -0.0546554848551750)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, 0)).x;
    outp.target1 += float4(-0.0011764382943511, -0.0299451071768999, 0.0229587312787771, 0.0021908886265010)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, inputPtY)).x;
    outp.target1 += float4(0.0098101310431957, 0.0080995410680771, -0.0030452020000666, -0.0132035519927740)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, 2 * inputPtY)).x;
    outp.target1 += float4(-0.0168330334126949, -0.0743711441755295, -0.0259261634200811, 0.0234480481594801)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, -2 * inputPtY)).x;
    outp.target1 += float4(0.0239933785051107, 0.1896541714668274, 0.0207756329327822, -0.0370332375168800)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, -inputPtY)).x;
    outp.target1 += float4(0.0094799501821399, -0.0652511194348335, -0.0004292793164495, -0.0726212188601494)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, 0)).x;
    outp.target1 += float4(0.0297284796833992, -0.1210186630487442, -0.0202929321676493, -0.0574462898075581)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, inputPtY)).x;
    outp.target1 += float4(-0.0318185277283192, 0.0840775370597839, 0.0110451309010386, 0.0415569432079792)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, 2 * inputPtY)).x;
    outp.target1 += float4(-0.0253141783177853, 0.1168256178498268, 0.1159729585051537, 0.0963164269924164)
		* yuvTex.Sample(samp, input.Tex + float2(0, -2 * inputPtY)).x;
    outp.target1 += float4(-0.1103615835309029, -0.0276833958923817, -0.4999594092369080, 0.1053867191076279)
		* yuvTex.Sample(samp, input.Tex + float2(0, -inputPtY)).x;
    outp.target1 += float4(1.1100435256958008, 0.0646764487028122, 0.0154005717486143, 0.8891586661338806)
		* yuvTex.Sample(samp, input.Tex).x;
    outp.target1 += float4(0.1229330673813820, 0.1719468832015991, 0.5730338096618652, -0.1645544171333313)
		* yuvTex.Sample(samp, input.Tex + float2(0, inputPtY)).x;
    outp.target1 += float4(-0.0090442728251219, -0.3023961782455444, -0.1589493155479431, 0.0418574027717113)
		* yuvTex.Sample(samp, input.Tex + float2(0, 2 * inputPtY)).x;
    outp.target1 += float4(0.0031942036002874, -0.1310926079750061, 0.0075543406419456, -0.0016449346439913)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, -2 * inputPtY)).x;
    outp.target1 += float4(-0.0995150282979012, -0.0701921209692955, -0.0130895879119635, 0.1344170123338699)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, -inputPtY)).x;
    outp.target1 += float4(0.0060519003309309, -0.1533465683460236, 0.0114194005727768, 0.0264683905988932)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, 0)).x;
    outp.target1 += float4(0.0244008023291826, 0.1881769001483917, -0.0206351149827242, -0.0628309547901154)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, inputPtY)).x;
    outp.target1 += float4(0.0075713125988841, 0.0508594363927841, 0.0430423170328140, -0.0124188791960478)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, 2 * inputPtY)).x;
    outp.target1 += float4(-0.0166875869035721, -0.0047865519300103, 0.0006719123339280, 0.0316803231835365)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, -2 * inputPtY)).x;
    outp.target1 += float4(-0.0058461269363761, 0.0990798473358154, -0.0177743826061487, -0.0066122291609645)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, -inputPtY)).x;
    outp.target1 += float4(-0.0972401946783066, -0.0225446373224258, -0.0037693574558944, 0.1953062713146210)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, 0)).x;
    outp.target1 += float4(-0.0216837190091610, -0.1824268400669098, 0.0069816261529922, 0.0283037684857845)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, inputPtY)).x;
    outp.target1 += float4(-0.0025767991319299, 0.0459827110171318, -0.0080216089263558, 0.0084134787321091)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, 2 * inputPtY)).x;

    outp.target2 = float4(0.0541447550058365, 0.0088306749239564, -0.0112389577552676, -0.0127860950306058);
    outp.target2 += float4(0.0142660010606050, 0.0137931071221828, 0.0061188107356429, -0.0104134222492576)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, -2 * inputPtY)).x;
    outp.target2 += float4(0.0147292809560895, -0.0289912857115269, 0.0266769435256720, 0.0933856964111328)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, -inputPtY)).x;
    outp.target2 += float4(-0.1734338253736496, 0.1116316691040993, -0.1973157376050949, -0.0581855811178684)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, 0)).x;
    outp.target2 += float4(0.0347507223486900, -0.0341566652059555, 0.0061667622067034, 0.0075258882716298)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, inputPtY)).x;
    outp.target2 += float4(0.0069884369149804, -0.0194250214844942, 0.0080830128863454, -0.0036874092184007)
		* yuvTex.Sample(samp, input.Tex + float2(-2 * inputPtX, 2 * inputPtY)).x;
    outp.target2 += float4(0.0233764201402664, 0.0344744995236397, 0.0162145942449570, 0.0979529991745949)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, -2 * inputPtY)).x;
    outp.target2 += float4(0.1280796974897385, -0.1018339172005653, -0.0132977198809385, -0.0019474622095004)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, -inputPtY)).x;
    outp.target2 += float4(0.4286882579326630, 0.1222677752375603, 0.7046694159507751, 0.0945475697517395)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, 0)).x;
    outp.target2 += float4(0.1107441782951355, -0.0134433070197701, -0.0174900908023119, -0.1686445474624634)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, inputPtY)).x;
    outp.target2 += float4(0.0321478620171547, 0.0065357843413949, 0.0300805997103453, 0.0420113280415535)
		* yuvTex.Sample(samp, input.Tex + float2(-inputPtX, 2 * inputPtY)).x;
    outp.target2 += float4(-0.1240341588854790, 0.0950303301215172, -0.0129648456349969, -0.2681856453418732)
		* yuvTex.Sample(samp, input.Tex + float2(0, -2 * inputPtY)).x;
    outp.target2 += float4(0.4846960902214050, 0.0351924635469913, 0.0223043337464333, -0.1273630708456039)
		* yuvTex.Sample(samp, input.Tex + float2(0, -inputPtY)).x;
    outp.target2 += float4(-1.9379507303237915, -0.2444442063570023, 0.0291962660849094, -0.3835578560829163)
		* yuvTex.Sample(samp, input.Tex).x;
    outp.target2 += float4(0.6396278142929077, -0.0765938311815262, -0.0552659817039967, 0.4393545985221863)
		* yuvTex.Sample(samp, input.Tex + float2(0, inputPtY)).x;
    outp.target2 += float4(-0.1969728022813797, -0.0607173256576061, 0.0131113547831774, 0.0542017817497253)
		* yuvTex.Sample(samp, input.Tex + float2(0, 2 * inputPtY)).x;
    outp.target2 += float4(0.0091696009039879, -0.0031533432193100, -0.0368777588009834, -0.0459998287260532)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, -2 * inputPtY)).x;
    outp.target2 += float4(0.1096992492675781, 0.2597902715206146, 0.0304869692772627, -0.0195200722664595)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, -inputPtY)).x;
    outp.target2 += float4(0.2889648377895355, -0.4275591969490051, -0.7414156794548035, 0.2695442438125610)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, 0)).x;
    outp.target2 += float4(0.0892018377780914, -0.0229137558490038, 0.0244414471089840, -0.1926898956298828)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, inputPtY)).x;
    outp.target2 += float4(0.0576358586549759, 0.0027846973389387, -0.0036861505359411, -0.0253547113388777)
		* yuvTex.Sample(samp, input.Tex + float2(inputPtX, 2 * inputPtY)).x;
    outp.target2 += float4(0.0159624069929123, 0.0319602824747562, 0.0019470085389912, 0.0089780492708087)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, -2 * inputPtY)).x;
    outp.target2 += float4(0.0552792511880398, 0.0543054342269897, 0.0134062822908163, 0.0545728243887424)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, -inputPtY)).x;
    outp.target2 += float4(-0.1170092225074768, 0.1963327825069427, 0.1503890156745911, 0.1891828328371048)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, 0)).x;
    outp.target2 += float4(-0.0084421783685684, 0.1297017931938171, -0.0330600887537003, -0.0942063704133034)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, inputPtY)).x;
    outp.target2 += float4(0.0118440408259630, -0.0337875857949257, 0.0055063469335437, 0.0254479162395000)
		* yuvTex.Sample(samp, input.Tex + float2(2 * inputPtX, 2 * inputPtY)).x;
    return outp;
}