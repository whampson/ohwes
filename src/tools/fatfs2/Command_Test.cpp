#include "Command.hpp"

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

    //
    // Invalid cases
    //

    CheckResult(SetShortName(e, "") == false);
    CheckResult(SetShortName(e, "dots..") == false);
    CheckResult(SetShortName(e, ".bar") == false);
    CheckResult(SetShortName(e, "itstoolong.txt") == false);
    CheckResult(SetShortName(e, "itstoo.long") == false);

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

int Test(const Command *cmd, const CommandArgs *args)
{
    (void) cmd;
    bool success = true;

    if (args->Argc < 2) {
        CheckResult(TestFatString());
        CheckResult(TestShortName());
    }
    else {
        if (strcmp("shortname", args->Argv[1]) == 0) {
            if (args->Argc < 3) {
                LogError("please provide a shortname to validate\n");
                return STATUS_ERROR;
            }
            success = ValidateShortName(args->Argv[2]);
        }
    }

    LogInfo("%s\n", (success) ? "Pass!" : "Fail!");
    return (success) ? STATUS_SUCCESS : STATUS_ERROR;
}

