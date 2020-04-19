//
// Created by liyinbin on 2020/1/25.
//
#include <abel/digest/sha512.h>
#include <abel/strings/hex_dump.h>
#include <gtest/gtest.h>

TEST(Sha512, all) {
    EXPECT_EQ(
            abel::sha512_hex_uc(""),
            "CF83E1357EEFB8BDF1542850D66D8007D620E4050B5715DC83F4A921D36CE9CE"
            "47D0D13C5D85F2B0FF8318D2877EEC2F63B931BD47417A81A538327AF927DA3E");

    EXPECT_EQ(
            abel::sha512_hex_uc("abc"),
            "DDAF35A193617ABACC417349AE20413112E6FA4E89A97EA20A9EEEE64B55D39A"
            "2192992A274FC1A836BA3C23A3FEEBBD454D4423643CE80E2A9AC94FA54CA49F");

    EXPECT_EQ(
            abel::sha512_hex_uc(
                    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
            "204A8FC6DDA82F0A0CED7BEB8E08A41657C16EF468B228A8279BE331A703C335"
            "96FD15C13B1B07F9AA1D3BEA57789CA031AD85C7A71DD70354EC631238CA3445");

    EXPECT_EQ(
            abel::sha512_hex_uc(
                    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
                    "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
            "8E959B75DAE313DA8CF4F72814FC143F8F7779C6EB9F7FA17299AEADB6889018"
            "501D289E4900F7E4331B99DEC4B5433AC7D329EEB6DD26545E96E55B874BE909");

    EXPECT_EQ(
            abel::sha512_hex_uc(std::string(1000000, 'a')),
            "E718483D0CE769644E2E42C7BC15B4638E1F98B13B2044285632A803AFA973EB"
            "DE0FF244877EA60A4CB0432CE577C31BEB009C5C2C49AA2E4EADB217AD8CC09B");

    std::vector<std::pair<const char *, const char *> > test_vectors = {
            {"21",
                    "3831a6a6155e509dee59a7f451eb35324d8f8f2df6e3708894740f98fdee2388"
                    "9f4de5adb0c5010dfb555cda77c8ab5dc902094c52de3278f35a75ebc25f093a"},
            {"9083",
                    "55586ebba48768aeb323655ab6f4298fc9f670964fc2e5f2731e34dfa4b0c09e"
                    "6e1e12e3d7286b3145c61c2047fb1a2a1297f36da64160b31fa4c8c2cddd2fb4"},
            {"0a55db",
                    "7952585e5330cb247d72bae696fc8a6b0f7d0804577e347d99bc1b11e52f3849"
                    "85a428449382306a89261ae143c2f3fb613804ab20b42dc097e5bf4a96ef919b"},
            {"23be86d5",
                    "76d42c8eadea35a69990c63a762f330614a4699977f058adb988f406fb0be8f2"
                    "ea3dce3a2bbd1d827b70b9b299ae6f9e5058ee97b50bd4922d6d37ddc761f8eb"},
            {"eb0ca946c1",
                    "d39ecedfe6e705a821aee4f58bfc489c3d9433eb4ac1b03a97e321a2586b40dd"
                    "0522f40fa5aef36afff591a78c916bfc6d1ca515c4983dd8695b1ec7951d723e"},
            {"38667f39277b",
                    "85708b8ff05d974d6af0801c152b95f5fa5c06af9a35230c5bea2752f031f9bd"
                    "84bd844717b3add308a70dc777f90813c20b47b16385664eefc88449f04f2131"},
            {"b39f71aaa8a108",
                    "258b8efa05b4a06b1e63c7a3f925c5ef11fa03e3d47d631bf4d474983783d8c0"
                    "b09449009e842fc9fa15de586c67cf8955a17d790b20f41dadf67ee8cdcdfce6"},
            {"6f8d58b7cab1888c",
                    "a3941def2803c8dfc08f20c06ba7e9a332ae0c67e47ae57365c243ef40059b11"
                    "be22c91da6a80c2cff0742a8f4bcd941bdee0b861ec872b215433ce8dcf3c031"},
            {"162b0cf9b3750f9438",
                    "ade217305dc34392aa4b8e57f64f5a3afdd27f1fa969a9a2608353f82b95cfb4"
                    "ae84598d01575a578a1068a59b34b5045ff6d5299c5cb7ee17180701b2d1d695"},
            {"bad7c618f45be207975e",
                    "5886828959d1f82254068be0bd14b6a88f59f534061fb20376a0541052dd3635"
                    "edf3c6f0ca3d08775e13525df9333a2113c0b2af76515887529910b6c793c8a5"},
            {"6213e10a4420e0d9b77037",
                    "9982dc2a04dff165567f276fd463efef2b369fa2fbca8cee31ce0de8a79a2eb0"
                    "b53e437f7d9d1f41c71d725cabb949b513075bad1740c9eefbf6a5c6633400c7"},
            {"6332c3c2a0a625a61df71858",
                    "9d60375d9858d9f2416fb86fa0a2189ee4213e8710314fd1ebed0fd158b043e6"
                    "e7c9a76d62c6ba1e1d411a730902309ec676dd491433c6ef66c8f116233d6ce7"},
            {"f47be3a2b019d1beededf5b80c",
                    "b94292625caa28c7be24a0997eb7328062a76d9b529c0f1d568f850df6d569b5"
                    "e84df07e9e246be232033ffac3adf2d18f92ab9dacfc0ecf08aff7145f0b833b"},
            {"b1715f782ff02c6b88937f054116",
                    "ee1a56ee78182ec41d2c3ab33d4c41871d437c5c1ca060ee9e219cb83689b4e5"
                    "a4174dfdab5d1d1096a31a7c8d3abda75c1b5e6da97e1814901c505b0bc07f25"},
            {"9bcd5262868cd9c8a96c9e82987f03",
                    "2e07662a001b9755ae922c8e8a95756db5341dc0f2e62ae1cf827038f33ce055"
                    "f63ad5c00b65391428434ddc01e5535e7fecbf53db66d93099b8e0b7e44e4b25"},
            {"cd67bd4054aaa3baa0db178ce232fd5a",
                    "0d8521f8f2f3900332d1a1a55c60ba81d04d28dfe8c504b6328ae787925fe018"
                    "8f2ba91c3a9f0c1653c4bf0ada356455ea36fd31f8e73e3951cad4ebba8c6e04"},
            {"6ba004fd176791efb381b862e298c67b08",
                    "112e19144a9c51a223a002b977459920e38afd4ca610bd1c532349e9fa7c0d50"
                    "3215c01ad70e1b2ac5133cf2d10c9e8c1a4c9405f291da2dc45f706761c5e8fe"},
            {"c6a170936568651020edfe15df8012acda8d",
                    "c36c100cdb6c8c45b072f18256d63a66c9843acb4d07de62e0600711d4fbe64c"
                    "8cf314ec3457c90308147cb7ac7e4d073ba10f0ced78ea724a474b32dae71231"},
            {"61be0c9f5cf62745c7da47c104597194db245c",
                    "b379249a3ca5f14c29456710114ba6f6136b34c3fc9f6fb91b59d491af782d6b"
                    "237eb71aaffdd38079461cf690a46d9a4ddd602d19808ab6235d1d8aa01e8200"},
            {"e07056d4f7277bc548099577720a581eec94141d",
                    "59f1856303ff165e2ab5683dddeb6e8ad81f15bb578579b999eb5746680f22cf"
                    "ec6dba741e591ca4d9e53904837701b374be74bbc0847a92179ac2b67496d807"},
            {"67ebda0a3573a9a58751d4169e10c7e8663febb3a8",
                    "13963f81cfabfca71de4739fd24a10ce3897bba1d716907fc0a28490c192a7fc"
                    "3ccb8db1f91af7a2d250d6617f0dfd1519d221d618a02e3e3fa9041cf35ed1ea"},
            {"63e09db99eb4cd6238677859a567df313c8520d845b4",
                    "9083e5348b08eb9810b2d15781d8265845410de54fe61750d4b93853690649ad"
                    "c6e72490bc2b7c365e2390573d9414becc0939719e0cb78eca6b2c80c2fda920"},
            {"f3e06b4bd79e380a65cb679a98ccd732563cc5ebe892e2",
                    "6b315f106b07c59eedc5ab1df813b3c0b903060e7217cc010e9070278512a885"
                    "008dac8b2472a521e77835a7f4deadc1d591aa23b624b69948a99bb60121c54e"},
            {"16b17074d3e3d97557f9ed77d920b4b1bff4e845b345a922",
                    "6884134582a760046433abcbd53db8ff1a89995862f305b887020f6da6c7b903"
                    "a314721e972bf438483f452a8b09596298a576c903c91df4a414c7bd20fd1d07"},
            {"3edf93251349d22806bed25345fd5c190aac96d6cdb2d758b8",
                    "299e0daf6605e5b0c30e1ec8bb98e7a3bd7b33b388bdb457452dab509594406c"
                    "8e7b841e6f4e75c8d6fbd614d5eb9e56c359bfafb4285754787ab72b46dd33f0"},
            {"b2d5a14f01e6b778888c562a059ec819ad89992d16a09f7a54b4",
                    "ab2e7d745d8ad393439af2a3fbc9cdc25510d4a04e78b526e12b1c0be3b22966"
                    "872ebe652e2f46ed5c5acecd2f233a9175dd295ebeb3a0706fc66fa1b137042b"},
            {"844b66f12ba0c5f9e92731f571539d1eef332e1549a49dbfa4c6de",
                    "c3f9c5781925774783ae9d839772d7513dfcea8c5af8da262c196f9fe80135b2"
                    "b0c8c6ca0a1604e0a3460247620de20b299f2db7871982d27c2176ae5fa7ad65"},
            {"6b6cc692d39860b1f30203653e25d09c01e6a8043c1a9cb8b249a41e",
                    "2e5263d9a4f21b210e0e161ed39df44102864325788647261a6e70ea4b1ee0ab"
                    "b57b57499bc82158d82336dd53f1ef4464c6a08126e138b2cc0892f765f6af85"},
            {"ab1fc9ee845eeb205ec13725daf1fb1f5d50629b14ea9a2235a9350a88",
                    "72d188a9df5f3b00057bca22c92c0f8228422d974302d22d4b322e7a6c8fc3b2"
                    "b50ec74c6842781f29f7075c3d4bd065878648846c39bb3e4e2692c0f053f7ed"},
            {"594ed82acfc03c0e359cc560b8e4b85f6ee77ee59a70023c2b3d5b3285b2",
                    "5ef322cb4014ecbb713a13659612a222225984d31c187debc4459ba7901f03da"
                    "c775400acfe3510b306b79894fb0e8437b412150c9193ee5a2164306ebb78301"},
            {"f2c66efbf2a76c5b041860ea576103cd8c6b25e50eca9ff6a2fa88083fe9ac",
                    "7978f93ef7ed02c4a24abecba124d14dd214e1492ff1e168304c0eab89637da0"
                    "f7a569c43dc4562bdb9404a018b6314fe0eebaccfb25ba76506aa7e9dcd956a7"},
            {"8ccb08d2a1a282aa8cc99902ecaf0f67a9f21cffe28005cb27fcf129e963f99d",
                    "4551def2f9127386eea8d4dae1ea8d8e49b2add0509f27ccbce7d9e950ac7db0"
                    "1d5bca579c271b9f2d806730d88f58252fd0c2587851c3ac8a0e72b4e1dc0da6"}
    };

    for (const auto &p : test_vectors) {
        EXPECT_EQ(abel::sha512_hex(abel::parse_hex_dump(p.first)), p.second);
    }
}
