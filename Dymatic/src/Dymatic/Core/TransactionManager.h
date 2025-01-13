#pragma once

#include "Dymatic/Core/Base.h"

#include <string>

namespace Dymatic {

	class Transaction
	{
	public:
		virtual ~Transaction() {}

		virtual void Execute() = 0;
		virtual void Undo() = 0;

		virtual const std::string GetName() const = 0;
	};

	class TransactionGroup : public Transaction
	{
	public:
		TransactionGroup(const std::string& name = "Transaction Group")
			: m_Name(name)
		{}

		virtual void Execute() override
		{
			// Execute transactions in the order they were added.
			for (auto& transaction : m_Transactions)
				transaction->Execute();
 		}

		virtual void Undo() override
		{
			// Execute transactions in the reverse order they were added for undo operations.
			for (auto it = m_Transactions.rbegin(); it != m_Transactions.rend(); ++it)
				(*it)->Undo();
		}

		void Add(Ref<Transaction> transaction)
		{
			m_Transactions.push_back(transaction);
		}

		virtual const std::string GetName() const override { return m_Name; }

	private:
		std::string m_Name;
		std::vector<Ref<Transaction>> m_Transactions;
	};

	class TransactionManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void Execute(Ref<Transaction> transaction);

		static void BeginTransaction(const std::string& name = "Unnamed Transaction");
		static void EndTransaction();

		static Ref<Transaction> Undo();
		static Ref<Transaction> Redo();

		static bool CanUndo();
		static bool CanRedo();

		static void ClearTransactions();
		static void ClearRedoStack();

	private:
		static void PushToStack(Ref<Transaction> transaction);
	};

}