static struct UnicodeKeyEntry {
    uint16_t unicode;
    uint32_t xKey;
} unicodeToX11[] = {
    { 0x20AC, XK_EuroSign },
    // Cyrillic symbols
    { 0x0410, XK_Cyrillic_A }, // capital letter a
    { 0x0430, XK_Cyrillic_a }, // small letter a
    { 0x0411, XK_Cyrillic_BE }, // capital letter be
    { 0x0431, XK_Cyrillic_be }, // small letter be
    { 0x0412, XK_Cyrillic_VE }, // capital letter ve
    { 0x0432, XK_Cyrillic_ve }, // small letter ve
    { 0x0413, XK_Cyrillic_GHE }, // capital letter ghe
    { 0x0433, XK_Cyrillic_ghe }, // small letter ghe
    { 0x0414, XK_Cyrillic_DE }, // capital letter de
    { 0x0434, XK_Cyrillic_de }, // small letter de
    { 0x0415, XK_Cyrillic_IE }, // capital letter ie
    { 0x0435, XK_Cyrillic_ie }, // small letter ie
    { 0x0401, XK_Cyrillic_IO }, // capital letter io
    { 0x0451, XK_Cyrillic_io }, // small letter io
    { 0x0416, XK_Cyrillic_ZHE }, // capital letter zhe
    { 0x0436, XK_Cyrillic_zhe }, // small letter zhe
    { 0x0417, XK_Cyrillic_ZE }, // capital letter ze
    { 0x0437, XK_Cyrillic_ze }, // small letter ze
    { 0x0418, XK_Cyrillic_I }, // capital letter i
    { 0x0438, XK_Cyrillic_i }, // small letter i
    { 0x0419, XK_Cyrillic_SHORTI }, // capital letter short i
    { 0x0439, XK_Cyrillic_shorti }, // small letter short i
    { 0x041A, XK_Cyrillic_KA }, // capital letter ka
    { 0x043A, XK_Cyrillic_ka }, // small letter ka
    { 0x041B, XK_Cyrillic_EL }, // capital letter el
    { 0x043B, XK_Cyrillic_el }, // small letter el
    { 0x041C, XK_Cyrillic_EM }, // capital letter em
    { 0x043C, XK_Cyrillic_em }, // small letter em
    { 0x041D, XK_Cyrillic_EN }, // capital letter en
    { 0x043D, XK_Cyrillic_en }, // small letter en
    { 0x041E, XK_Cyrillic_O }, // capital letter o
    { 0x043E, XK_Cyrillic_o }, // small letter o
    { 0x041F, XK_Cyrillic_PE }, // capital letter pe
    { 0x043F, XK_Cyrillic_pe }, // small letter pe
    { 0x0420, XK_Cyrillic_ER }, // capital letter er
    { 0x0440, XK_Cyrillic_er }, // small letter er
    { 0x0421, XK_Cyrillic_ES }, // capital letter es
    { 0x0441, XK_Cyrillic_es }, // small letter es
    { 0x0422, XK_Cyrillic_TE }, // capital letter te
    { 0x0442, XK_Cyrillic_te }, // small letter te
    { 0x0423, XK_Cyrillic_U }, // capital letter u
    { 0x0443, XK_Cyrillic_u }, // small letter u
    { 0x0424, XK_Cyrillic_EF }, // capital letter ef
    { 0x0444, XK_Cyrillic_ef }, // small letter ef
    { 0x0425, XK_Cyrillic_HA }, // capital letter ha
    { 0x0445, XK_Cyrillic_ha }, // small letter ha
    { 0x0426, XK_Cyrillic_TSE }, // capital letter tse
    { 0x0446, XK_Cyrillic_tse }, // small letter tse
    { 0x0427, XK_Cyrillic_CHE }, // capital letter che
    { 0x0447, XK_Cyrillic_che }, // small letter che
    { 0x0428, XK_Cyrillic_SHA }, // capital letter sha
    { 0x0448, XK_Cyrillic_sha }, // small letter sha
    { 0x0429, XK_Cyrillic_SHCHA }, // capital letter shcha
    { 0x0449, XK_Cyrillic_shcha }, // small letter shcha
    { 0x042A, XK_Cyrillic_HARDSIGN }, // capital letter hard sign
    { 0x044A, XK_Cyrillic_hardsign }, // small letter hard sign
    { 0x042B, XK_Cyrillic_YERU }, // capital letter yeru
    { 0x044B, XK_Cyrillic_yeru }, // small letter yeru
    { 0x042C, XK_Cyrillic_SOFTSIGN }, // capital letter soft sign
    { 0x044C, XK_Cyrillic_softsign }, // small letter soft sign
    { 0x042D, XK_Cyrillic_E }, // capital letter e
    { 0x044D, XK_Cyrillic_e }, // small letter e
    { 0x042E, XK_Cyrillic_YU }, // capital letter yu
    { 0x044E, XK_Cyrillic_yu }, // small letter yu
    { 0x042F, XK_Cyrillic_YA }, // capital letter ya
    { 0x044F, XK_Cyrillic_ya }, // small letter ya
    // знаки для современных славянских языков
    { 0x0402, XK_Serbian_DJE }, // Ђ capital letter dje (serbocroatian)
    { 0x0452, XK_Serbian_dje }, // ђ small letter dje (serbocroatian)
    { 0x0403, XK_Macedonia_GJE }, // Ѓ capital letter gje
    { 0x0453, XK_Macedonia_gje }, // ѓ small letter gje
    { 0x0404, XK_Ukrainian_IE }, // Є capital letter ukrainian ie
    { 0x0454, XK_Ukrainian_ie }, // є small letter ukrainian ie
    { 0x0405, XK_Cyrillic_DZHE }, // Ѕ capital letter dze
    { 0x0455, XK_Cyrillic_dzhe }, // ѕ small letter dze
    { 0x0406, XK_Ukrainian_I }, // І capital letter byelorussian-ukrainian i
    { 0x0456, XK_Ukrainian_i }, // і small letter byelorussian-ukrainian i
    { 0x0407, XK_Ukrainian_YI }, // Ї capital letter yi (ukrainian)
    { 0x0457, XK_Ukrainian_yi }, // ї small letter yi (ukrainian)
    { 0x0408, XK_Cyrillic_JE }, // Ј capital letter je
    { 0x0458, XK_Cyrillic_je }, // ј small letter je
    { 0x0409, XK_Cyrillic_LJE }, // Љ capital letter lje
    { 0x0459, XK_Cyrillic_lje }, // љ small letter lje
    { 0x040A, XK_Cyrillic_NJE }, // Њ capital letter nje
    { 0x045A, XK_Cyrillic_nje }, // њ small letter nje
    { 0x040B, XK_Serbian_TSHE }, // Ћ capital letter tshe (serbocroatian)
    { 0x045B, XK_Serbian_tshe }, // ћ small letter tshe (serbocroatian)
    { 0x040C, XK_Macedonia_KJE }, // Ќ capital letter kje
    { 0x045C, XK_Macedonia_kje }, // ќ small letter kje
    //{ 0x040D, XK_Cyrillic_ }, // Ѝ capital letter i with grave
    //{ 0x045D, XK_Cyrillic_ }, // ѝ small letter i with grave
    { 0x040E, XK_Byelorussian_SHORTU }, // Ў capital letter short u (byelorussian)
    { 0x045E, XK_Byelorussian_shortu }, // ў small letter short u (byelorussian)
    { 0x040F, XK_Cyrillic_DZHE }, // Џ capital letter dzhe
    { 0x045F, XK_Cyrillic_dzhe }, // џ small letter dzhe
    // { 0x0490, XK_ }, // Ґ capital letter ghe with upturn
    // { 0x0491, XK_ }, // ґ small letter ghe with upturn
    { 0, 0}
};
