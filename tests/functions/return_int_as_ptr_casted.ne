function get7AsPtr(): uint8*
{
    return to<uint8*>(7);
}

function main(): int32
{
    return to<int32>(get7AsPtr());
}
