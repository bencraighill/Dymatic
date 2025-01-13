#include "dypch.h"

#include "Dymatic/Core/TransactionManager.h"

#include <stack>

namespace Dymatic {
	
	static std::stack<Ref<Transaction>> s_ExecuteStack;
	static std::stack<Ref<Transaction>> s_RedoStack;

	static Ref<TransactionGroup> s_ActiveTransactionGroup = nullptr;

	void TransactionManager::Init()
	{
	}

	void TransactionManager::Shutdown()
	{
	}

	void TransactionManager::Execute(Ref<Transaction> transaction)
	{
		// Push the transaction to the active group if it exists, otherwise the main stack.
		if (s_ActiveTransactionGroup)
			s_ActiveTransactionGroup->Add(transaction);
		else
			PushToStack(transaction);


		// Execute the transaction.
		transaction->Execute();
	}

	void TransactionManager::BeginTransaction(const std::string& name)
	{
		// If we already have called 'BeginTransaction' do not create a new transaction as all of this 
		// can be merged into one transaction.
		if (s_ActiveTransactionGroup)
			return;

		// Otherwise create the transaction group and push it to the stack.
		s_ActiveTransactionGroup = CreateRef<TransactionGroup>(name);
		PushToStack(s_ActiveTransactionGroup);
	}

	void TransactionManager::EndTransaction()
	{
		s_ActiveTransactionGroup = nullptr;
	}

	Ref<Transaction> TransactionManager::Undo()
	{
		if (s_ExecuteStack.empty())
			return nullptr;

		// Push the transaction onto the redo stack and undo it.
		s_RedoStack.push(s_ExecuteStack.top());
		s_ExecuteStack.top()->Undo();
		s_ExecuteStack.pop();

		DY_CORE_INFO("Redo Transaction: {}", s_RedoStack.top()->GetName());

		return s_RedoStack.top();
	}

	Ref<Transaction> TransactionManager::Redo()
	{
		if (s_RedoStack.empty())
			return nullptr;

		// Push the transaction onto the execute stack and execute it.
		s_ExecuteStack.push(s_RedoStack.top());
		s_RedoStack.top()->Execute();
		s_RedoStack.pop();

		DY_CORE_INFO("Redo Transaction: {}", s_ExecuteStack.top()->GetName());

		return s_ExecuteStack.top();
	}

	bool TransactionManager::CanUndo()
	{
		return !s_ExecuteStack.empty();
	}

	bool TransactionManager::CanRedo()
	{
		return !s_RedoStack.empty();
	}

	void TransactionManager::ClearTransactions()
	{
		while (!s_ExecuteStack.empty())
			s_ExecuteStack.pop();

		ClearRedoStack();
	}

	void TransactionManager::ClearRedoStack()
	{
		while (!s_RedoStack.empty())
			s_RedoStack.pop();
	}

	void TransactionManager::PushToStack(Ref<Transaction> transaction)
	{
		// Push the transaction onto the stack and execute it.
		s_ExecuteStack.push(transaction);

		// Remove all transactions on the redo stack after a transaction has been made.
		ClearRedoStack();
	}

}