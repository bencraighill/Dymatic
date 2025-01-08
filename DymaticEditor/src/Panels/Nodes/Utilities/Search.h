#pragma once

#include <string>
#include <map>
#include <functional>

#include "Dymatic/Math/StringUtils.h"

#include "Dymatic/Editor/EditorGraph.h"

namespace Dymatic {

	struct NodeSearchResult
	{
		NodeSearchResult() = default;
		NodeSearchResult(const std::function<Editor::NodeHandle()>& callback)
			: Callback(callback)
		{}

		std::function<Editor::NodeHandle()> Callback;
	};

	struct NodeSearchTree
	{
		// Use a map to maintain alphabetical order
		std::map<std::string, NodeSearchTree> Subtrees;
		std::map<std::string, NodeSearchResult> Results;

		NodeSearchTree& AddTree(const std::string& name)
		{
			Subtrees[name] = NodeSearchTree();
			return Subtrees.at(name);
		}

		void AddResult(const std::string& name, std::function<Editor::NodeHandle()> callback)
		{
			Results[name] = NodeSearchResult(callback);
		}

		void Clear()
		{
			Subtrees.clear();
			Results.clear();
		}
	};

	template <typename T>
	class NodeSearchTreeBuilder
	{
	public:
		NodeSearchTreeBuilder(NodeSearchTree* tree, const std::string& query)
			: m_SearchTree(tree)
		{
			m_SearchTree->Clear();

			if (!query.empty())
				String::SplitStringByDelimiter(String::ToLower(query), m_QueryWords, ' ');
		}

		void SetContext(T pinType, Editor::PinKind pinKind)
		{
			m_ContextSensitive = true;
			m_PinType = pinType;
			m_PinKind = pinKind;
		}

		void SetCompatibilityCallback(const std::function<bool(T, T)>& callback)
		{
			m_CompatibilityCallback = callback;
		}

		void AddResult(const std::string& name, const std::vector<std::string>& categories, const std::string& keywords, const std::vector<T>& inputs, const std::vector<T>& outputs, std::function<Editor::NodeHandle()> callback)
		{
			bool visible = true;

			if (!m_QueryWords.empty())
			{
				visible = false;

				// Note: Constructor already ensured the query is all lowercase
				std::string keywordString = String::ToLower(keywords);
				std::string functionName = String::ToLower(name);

				for (auto& query : m_QueryWords)
				{
					if (query.empty())
						continue;

					if (keywordString.find(query) != std::string::npos || functionName.find(query) != std::string::npos)
					{
						visible = true;
						break;
					}
				}
			}

			if (visible && m_ContextSensitive)
			{
				// Check if a pin of the opposite kind but same type exists on the other node to connect to
				visible = false;
				for (auto& param : (m_PinKind == Editor::PinKind::Input ? outputs : inputs))
				{
					if (m_CompatibilityCallback ? m_CompatibilityCallback(param, m_PinType) : param == m_PinType)
					{
						visible = true;
						break;
					}
				}
			}

			if (!visible)
				return;

			// Nodes can belong to multiple categories and each of those categories can have subcategories (subtrees)
			for (auto& category : categories)
			{
				// Split Category Into Segments (if required)
				std::vector<std::string> seglist;
				String::SplitStringByDelimiter(category, seglist, '|');

				// Find Corresponding Category
				NodeSearchTree* searchData = m_SearchTree;
				for (auto& segment : seglist)
				{
					// Remove Empty Categories
					if (segment.empty())
						continue;

					// Check if segment exists in current search data scope
					bool found = false;
					for (auto& [name, subtree] : searchData->Subtrees)
					{
						if (name == segment)
						{
							searchData = &subtree;
							found = true;
							break;
						}
					}

					// If function doesn't exist create new data scope and update to current one
					if (!found)
						searchData = &searchData->AddTree(segment);
				}

				// Add Function To Corresponding Category
				searchData->AddResult(name, callback);
			}
		}

	private:
		NodeSearchTree* m_SearchTree;

		std::function<bool(T, T)> m_CompatibilityCallback = nullptr;
		std::vector<std::string> m_QueryWords;
		bool m_ContextSensitive = false;
		T m_PinType;
		Editor::PinKind m_PinKind;
	};

}