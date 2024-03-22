#include "Command.hpp"
#include <wchar.h>

#define CheckResult(exp)                                                        \
({                                                                              \
    success &= (exp);                                                           \
    if (!success) {                                                             \
        LogError("failed: %s (%s:%d)\n", #exp, __FILE__, __LINE__);             \
    }                                                                           \
})

static bool TestFatString()
{
    char fStr[MAX_LABEL];
    char cStr[MAX_LABEL];

    bool success = true;

    WriteFatString(fStr, "", LABEL_LENGTH);
    ReadFatString(cStr, fStr, LABEL_LENGTH);
    CheckResult(strncmp(fStr, "           ", LABEL_LENGTH) == 0);
    CheckResult(strncmp(cStr, "",            LABEL_LENGTH) == 0);

    WriteFatString(fStr, "MAXIMUMLENG", LABEL_LENGTH);
    ReadFatString(cStr, fStr, LABEL_LENGTH);
    CheckResult(strncmp(fStr, "MAXIMUMLENG", LABEL_LENGTH) == 0);
    CheckResult(strncmp(cStr, "MAXIMUMLENG", LABEL_LENGTH) == 0);

    WriteFatString(fStr, "EXCEEDSMAXIMUM", LABEL_LENGTH);
    ReadFatString(cStr, fStr, LABEL_LENGTH);
    CheckResult(strncmp(fStr, "EXCEEDSMAXI", LABEL_LENGTH) == 0);
    CheckResult(strncmp(cStr, "EXCEEDSMAXI", LABEL_LENGTH) == 0);

    WriteFatString(fStr, "TRAILING", LABEL_LENGTH);
    ReadFatString(cStr, fStr, LABEL_LENGTH);
    CheckResult(strncmp(fStr, "TRAILING   ", LABEL_LENGTH) == 0);
    CheckResult(strncmp(cStr, "TRAILING",    LABEL_LENGTH) == 0);

    WriteFatString(fStr, "    LEADING", LABEL_LENGTH);
    ReadFatString(cStr, fStr, LABEL_LENGTH);
    CheckResult(strncmp(fStr, "    LEADING", LABEL_LENGTH) == 0);
    CheckResult(strncmp(cStr, "LEADING",     LABEL_LENGTH) == 0);

    WriteFatString(fStr, "SP ACE", LABEL_LENGTH);
    ReadFatString(cStr, fStr, LABEL_LENGTH);
    CheckResult(strncmp(fStr, "SP ACE     ", LABEL_LENGTH) == 0);
    CheckResult(strncmp(cStr, "SP ACE",      LABEL_LENGTH) == 0);

    return success;
}

static bool TestShortName()
{
    char buf[MAX_SHORTNAME];

    DirEntry dir;
    DirEntry *e = &dir;
    InitDirEntry(e);

    bool success = true;

    //
    // Valid cases
    //

    CheckResult(SetShortName(e, "foo.bar") == true);
    CheckResult(strncmp(e->Label, "FOO     BAR", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "FOO.BAR") == 0);

    CheckResult(SetShortName(e, "Foo") == true);
    CheckResult(strncmp(e->Label, "FOO        ", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "FOO") == 0);

    CheckResult(SetShortName(e, "Foo.") == true);
    CheckResult(strncmp(e->Label, "FOO        ", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "FOO") == 0);

    CheckResult(SetShortName(e, "PICKLE.A") == true);
    CheckResult(strncmp(e->Label, "PICKLE  A  ", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "PICKLE.A") == 0);

    CheckResult(SetShortName(e, "prettybg.big") == true);
    CheckResult(strncmp(e->Label, "PRETTYBGBIG", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "PRETTYBG.BIG") == 0);

    CheckResult(SetShortName(e, "%$#^&()~.`!@") == true);
    CheckResult(strncmp(e->Label, "%$#^&()~`!@", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "%$#^&()~.`!@") == 0);

    CheckResult(SetShortName(e, "\xE5" "aaaa.bbb") == true);
    CheckResult(strncmp(e->Label, "\x05" "AAAA   BBB", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "\xE5" "AAAA.BBB") == 0);

    CheckResult(SetShortName(e, "  leading.spc") == true);
    CheckResult(strncmp(e->Label, "LEADING SPC", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "LEADING.SPC") == 0);

    CheckResult(SetShortName(e, "trailing.spc  ") == true);
    CheckResult(strncmp(e->Label, "TRAILINGSPC", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "TRAILING.SPC") == 0);

    CheckResult(SetShortName(e, "SP  ACE.txt") == true);        // technically allowed though not common
    CheckResult(strncmp(e->Label, "SP  ACE TXT", LABEL_LENGTH) == 0);
    CheckResult(strcmp(GetShortName(buf, e), "SP  ACE.TXT") == 0);

    //
    // Invalid cases
    //

    CheckResult(SetShortName(e, "") == false);
    CheckResult(SetShortName(e, "dots..") == false);
    CheckResult(SetShortName(e, "dot.dot.txt") == false);
    CheckResult(SetShortName(e, ".bar") == false);
    CheckResult(SetShortName(e, "thisisjusttoolong") == false);
    CheckResult(SetShortName(e, "filenamel.ong") == false);
    CheckResult(SetShortName(e, "exttoo.long") == false);
    CheckResult(SetShortName(e, "exttoo.long") == false);
    CheckResult(SetShortName(e, "*.txt") == false);
    CheckResult(SetShortName(e, "/:<.|+,") == false);
    CheckResult(SetShortName(e, "[COOL].TXT") == false);
    CheckResult(SetShortName(e, "HUH?.DOC") == false);
    CheckResult(SetShortName(e, "C++.cpp") == false);
    CheckResult(SetShortName(e, "<o>") == false);
    CheckResult(SetShortName(e, "a=b;") == false);

    // Not Allowed: " * / : < > ? \ | + , . ; = [ ]

    return success;
}

static bool ValidateShortName(const char *name)
{
    DirEntry dir;
    DirEntry *e = &dir;
    InitDirEntry(e);

    bool success = SetShortName(e, name);
    if (success)
        LogInfo("'%s' => '%*s'\n", name, LABEL_LENGTH, e->Label);
    else
        LogInfo("'%s' => (invalid)\n", name);

    return success;
}

static bool TestLongName()
{
    bool success = true;

    DirEntry dir;
    DirEntry *e = &dir;
    InitDirEntry(e);

    const char *sfn = "MYCOOL~1.TXT";
    const wchar_t *lfn = L"MyCoolFileWithAnAbnormallyLongName.txt";

    CheckResult(SetShortName(e, sfn));

    DirEntry dirTable[32];
    DirEntry *pTable;
    wchar_t lfnStrBuf[MAX_LONGNAME];

    // Test 1: Make sure we can set and read a long file name
    {
        pTable = dirTable;

        CheckResult(SetLongName(&pTable, lfn, e));
        const DirEntry *sfnEntry = GetLongName(lfnStrBuf, dirTable);

        // Compare LFN strings
        CheckResult(wcsncmp(lfn, lfnStrBuf, LONGNAME_LENGTH) == 0);

        // Compare SFN dir entries (should all be the same one)
        CheckResult(sfnEntry == pTable);
        CheckResult(memcmp(e, sfnEntry, sizeof(DirEntry)) == 0);
    }

    // Test 2: Fudge the checksum and ensure it returns an empty string
    {
        pTable = dirTable;

        CheckResult(SetLongName(&pTable, lfn, e));
        ((LongFileName *) &dirTable[1])->Checksum = 0x42;
        const DirEntry *sfnEntry = GetLongName(lfnStrBuf, dirTable);

        CheckResult(wcsncmp(L"", lfnStrBuf, LONGNAME_LENGTH) == 0);

        CheckResult(sfnEntry == pTable);
        CheckResult(memcmp(e, sfnEntry, sizeof(DirEntry)) == 0);
    }

    // Test 2: Try some empty strings, should return false.
    {
        pTable = dirTable;
        CheckResult(SetLongName(&pTable, L"", e) == false);

        pTable = dirTable;
        CheckResult(SetLongName(&pTable, L"     ", e) == false);

        pTable = dirTable;
        CheckResult(SetLongName(&pTable, L"but spaces are ok", e) == true);
        GetLongName(lfnStrBuf, dirTable);
        CheckResult(wcsncmp(L"but spaces are ok", lfnStrBuf, LONGNAME_LENGTH) == 0);
    }

    // Test 3: Leading spaces should be ignored
    {
        pTable = dirTable;
        CheckResult(SetLongName(&pTable, L"   This name contains leading spaces", e) == true);
        GetLongName(lfnStrBuf, dirTable);
        CheckResult(wcsncmp(L"This name contains leading spaces", lfnStrBuf, LONGNAME_LENGTH) == 0);
    }

    // Test 4: Trailing spaces should be ignored
    {
        pTable = dirTable;
        CheckResult(SetLongName(&pTable, L"This name contains trailing spaces      ", e) == true);
        GetLongName(lfnStrBuf, dirTable);
        CheckResult(wcsncmp(L"This name contains trailing spaces", lfnStrBuf, LONGNAME_LENGTH) == 0);
    }

    // Test 5: Leading dots are allowed, trailing dots are ignored
    {
        pTable = dirTable;
        CheckResult(SetLongName(&pTable, L".dotfile", e) == true);
        GetLongName(lfnStrBuf, dirTable);
        CheckResult(wcsncmp(L".dotfile", lfnStrBuf, LONGNAME_LENGTH) == 0);

        pTable = dirTable;
        CheckResult(SetLongName(&pTable, L"trailing dots.txt..", e) == true);
        GetLongName(lfnStrBuf, dirTable);
        CheckResult(wcsncmp(L"trailing dots.txt", lfnStrBuf, LONGNAME_LENGTH) == 0);
    }

    // Test 6: Invalid chars, these should fail
    {
        pTable = dirTable;
        CheckResult(SetLongName(&pTable, L"*.txt", e) == false);
        CheckResult(pTable == dirTable);

        CheckResult(SetLongName(&pTable, L"my/file/lives/here", e) == false);
        CheckResult(pTable == dirTable);

        CheckResult(SetLongName(&pTable, L"C:\\", e) == false);
        CheckResult(pTable == dirTable);

        CheckResult(SetLongName(&pTable, L"<iostream>", e) == false);
        CheckResult(pTable == dirTable);

        CheckResult(SetLongName(&pTable, L"What's wrong with the question mark?", e) == false);
        CheckResult(pTable == dirTable);

        CheckResult(SetLongName(&pTable, L"hash|pipe", e) == false);
        CheckResult(pTable == dirTable);
    }

    // Test 6: Maximum file name length
    {
        lfn = L"ThisIsASuperExtraExtremelyLongFileNameToSeeHowThisCodeHandles"
              L"VerySuperExtraCrazyUngodlyLongFileNamesNobodyShouldEverNameAFile"
              L"WithThisManyCharactersUnlessTheyAreLikeMeAndWantToSeeThingsBreak"
              L"MuhahahJesusWeStillArentAtTheLimitThisIsInsaneOhMyBoyManGodShi.txt";

        pTable = dirTable;
        CheckResult(SetLongName(&pTable, lfn, e) == true);
        GetLongName(lfnStrBuf, dirTable);
        CheckResult(wcsncmp(lfn, lfnStrBuf, LONGNAME_LENGTH) == 0);
    }

    // Test 7: Exceeds maximum file name length
    {
        wchar_t veryLongFileName[MAX_LONGNAME + 1];
        memset(veryLongFileName, 0, sizeof(veryLongFileName));
        swprintf(veryLongFileName, MAX_LONGNAME + 1, L"%0*d", MAX_LONGNAME + 1, 0);

        pTable = dirTable;
        CheckResult(SetLongName(&pTable, veryLongFileName, e) == false);
    }

    return success;
}

int Test(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    bool success = true;

    if (args->Argc < 2) {
        CheckResult(TestFatString());
        CheckResult(TestShortName());
        CheckResult(TestLongName());
        // TODO: write/read file then diff with original
        goto Done;
    }

    if (strcmp("shortname", args->Argv[1]) == 0) {
        if (args->Argc < 3) {
            LogError("please provide a shortname to validate\n");
            return STATUS_ERROR;
        }
        success = ValidateShortName(args->Argv[2]);
    }

Done:
    LogInfo("%s\n", (success) ? "Pass!" : "Fail!");
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}

