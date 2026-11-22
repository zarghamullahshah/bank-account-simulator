#include "bank.h"

#include <cmath>
#include <sstream>

namespace bank {

Account::Account(std::string id, std::string owner, Cents opening_balance_cents)
    : id_(std::move(id)),
      owner_(std::move(owner)),
      balance_cents_(opening_balance_cents < 0 ? 0 : opening_balance_cents) {
    if (opening_balance_cents > 0) {
        history_.push_back({"deposit", opening_balance_cents, balance_cents_,
                            "opening balance"});
    }
}

bool Account::deposit(Cents amount_cents, const std::string& note,
                      const std::string& type) {
    if (amount_cents <= 0) return false;
    balance_cents_ += amount_cents;
    history_.push_back({type, amount_cents, balance_cents_, note});
    return true;
}

bool Account::withdraw(Cents amount_cents, const std::string& note,
                       const std::string& type) {
    if (amount_cents <= 0 || amount_cents > balance_cents_) return false;
    balance_cents_ -= amount_cents;
    history_.push_back({type, amount_cents, balance_cents_, note});
    return true;
}

std::string Account::format(Cents amount_cents) {
    std::ostringstream out;
    if (amount_cents < 0) out << '-';
    const Cents abs_amount = std::abs(amount_cents);
    out << '$' << (abs_amount / 100) << '.';
    const Cents cents_part = abs_amount % 100;
    if (cents_part < 10) out << '0';
    out << cents_part;
    return out.str();
}

bool Bank::create_account(const std::string& id, const std::string& owner,
                          Cents opening_balance_cents) {
    if (find_account(id) != nullptr) return false;
    accounts_.emplace_back(id, owner, opening_balance_cents);
    return true;
}

Account* Bank::find_account(const std::string& id) {
    for (auto& account : accounts_) {
        if (account.id() == id) return &account;
    }
    return nullptr;
}

const Account* Bank::find_account(const std::string& id) const {
    for (const auto& account : accounts_) {
        if (account.id() == id) return &account;
    }
    return nullptr;
}

bool Bank::transfer(const std::string& from_id, const std::string& to_id,
                    Cents amount_cents) {
    if (amount_cents <= 0 || from_id == to_id) return false;
    Account* from = find_account(from_id);
    Account* to = find_account(to_id);
    if (from == nullptr || to == nullptr) return false;
    if (amount_cents > from->balance_cents()) return false;
    // Withdraw first; it cannot fail after the checks above, so the
    // transfer is atomic.
    from->withdraw(amount_cents, "transfer to " + to_id, "transfer-out");
    to->deposit(amount_cents, "transfer from " + from_id, "transfer-in");
    return true;
}

}  // namespace bank
