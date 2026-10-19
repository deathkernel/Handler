#include "handler/recovery_journal.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>

int main() {
    using namespace handler;

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto journalPath = std::filesystem::temp_directory_path() /
        ("handler-legacy-journal-" + std::to_string(nonce) + ".log");
    RecoveryJournal journal(journalPath);

    assert(journal.record("START", "first legacy transaction"));
    assert(journal.record("ACTION_BEGIN", "first legacy transaction"));
    assert(journal.record("START", "second legacy transaction"));
    assert(journal.record("ACTION_BEGIN", "second legacy transaction"));
    assert(journal.record("ROLLBACK", "first legacy transaction"));

    assert(!journal.hasCorruptEntries());
    assert(journal.hasUnfinishedTransaction());
    const auto stillActive = journal.unfinishedTransactionIds();
    assert(stillActive.size() == 1 && stillActive.front().empty());

    assert(journal.record("COMMIT", "second legacy transaction"));
    assert(!journal.hasUnfinishedTransaction());
    return 0;
}
