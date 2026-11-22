#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bank {

// Money is stored as integer cents so balances never suffer float rounding.
using Cents = std::int64_t;

struct Transaction {
    std::string type;        // "deposit", "withdraw", "transfer-in", "transfer-out"
    Cents amount_cents;
    Cents balance_after_cents;
    std::string note;
};

class Account {
public:
    Account(std::string id, std::string owner, Cents opening_balance_cents = 0);

    const std::string& id() const { return id_; }
    const std::string& owner() const { return owner_; }
    Cents balance_cents() const { return balance_cents_; }
    const std::vector<Transaction>& history() const { return history_; }

    // Returns false (and changes nothing) when amount <= 0.
    bool deposit(Cents amount_cents, const std::string& note = "",
                 const std::string& type = "deposit");
    // Returns false (and changes nothing) when amount <= 0 or exceeds balance.
    bool withdraw(Cents amount_cents, const std::string& note = "",
                  const std::string& type = "withdraw");

    static std::string format(Cents amount_cents);  // e.g. -1234 -> "-$12.34"

private:
    std::string id_;
    std::string owner_;
    Cents balance_cents_;
    std::vector<Transaction> history_;
};

class Bank {
public:
    // Returns false when an account with this id already exists.
    bool create_account(const std::string& id, const std::string& owner,
                        Cents opening_balance_cents = 0);
    Account* find_account(const std::string& id);
    const Account* find_account(const std::string& id) const;

    // Atomic: either both legs happen or neither does. False when either
    // account is missing, amount <= 0, or the source lacks funds.
    bool transfer(const std::string& from_id, const std::string& to_id,
                  Cents amount_cents);

    std::size_t account_count() const { return accounts_.size(); }

private:
    std::vector<Account> accounts_;
};

}  // namespace bank
