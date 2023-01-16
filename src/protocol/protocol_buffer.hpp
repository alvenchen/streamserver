


/*

c1 : client1 read pos
c2 : client2 read pos

s : server buffer

[ -> ...c1...c2        buffer          s... -> appending]

if ci slow (ie. s close to buffer.size())
    skip to next key frame


*/