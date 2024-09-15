#include "ContentBrowserPanel.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Project/Project.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"

#include "Panels/PanelManager.h"
#include "EditorResources.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include <string_view>
#include <queue>


namespace Athena
{
	ContentBrowserPanel::ContentBrowserPanel(const Ref<EditorContext>& context)
		: Panel(CONTENT_BROWSER_PANEL_ID, context), m_CurrentNode(nullptr)
	{
		Refresh();
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");
		
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 5.f, 10.f });
		ImGui::PushStyleColor(ImGuiCol_Button, UI::GetTheme().BackgroundDark);

		if (ImGui::ImageButton(UI::GetTextureID(EditorResources::GetIcon("ContentBrowser_Undo")), m_ButtonSize))
		{
			if (m_CurrentNode->ParentNode != nullptr)
			{
				m_CurrentNode = m_CurrentNode->ParentNode;
			}
		}

		ImGui::SameLine();

		if (ImGui::ImageButton(UI::GetTextureID(EditorResources::GetIcon("ContentBrowser_Redo")), m_ButtonSize))
		{
			// TODO
		}

		ImGui::SameLine();

		if (ImGui::ImageButton(UI::GetTextureID(EditorResources::GetIcon("ContentBrowser_Refresh")), m_ButtonSize))
		{
			Refresh();
		}

		ImGui::SameLine();

		ImVec2 regionAvail = ImGui::GetContentRegionAvail();

		ImGui::PushItemWidth(regionAvail.x * 0.15f);

		String oldString = m_SearchString;
		UI::TextInputWithHint("SearchTextInput", "Search...", m_SearchString);

		if (oldString != m_SearchString && !m_SearchString.empty())
		{
			Search();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + regionAvail.x * 0.01f);
		ImGui::Text(m_CurrentNode->FilePath.c_str());

		ImGui::Separator();

		bool showSearchResult = !m_SearchString.empty();
		uint32 nodeCount = showSearchResult ? m_SearchResult.size() : m_CurrentNode->Children.size();

		float cellSize = m_Padding + m_ItemSize.x;
		float panelWidth = regionAvail.x;

		ImGui::Columns(int(panelWidth / cellSize), 0, false);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
		for (uint32 i = 0; i < nodeCount; ++i)
		{
			auto& node = showSearchResult ? *m_SearchResult[i] : m_CurrentNode->Children[i];
			const auto& filename = node.FileName;

			ImGui::PushID(filename.data());
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });

			Ref<Texture2D> icon = node.IsFolder ? EditorResources::GetIcon("ContentBrowser_Folder") : EditorResources::GetIcon("ContentBrowser_File");
			ImGui::ImageButton(UI::GetTextureID(icon), m_ItemSize);

			bool enteredIntoFolder = false;
			if (node.IsFolder && ImGui::IsItemClicked())
			{
				enteredIntoFolder = true;
			}
			else
			{
				if (ImGui::BeginDragDropSource())
				{
					const auto& path = node.FilePath;

					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", path.data(), strlen(path.data()) + 1, ImGuiCond_Once);
					ImGui::Text(path.c_str());
					ImGui::EndDragDropSource();
				}
			}
			ImGui::PopStyleColor();
			ImGui::PopID();

			ImGui::TextWrapped(filename.data());
			ImGui::NextColumn();

			if (enteredIntoFolder)
			{
				if (showSearchResult)
				{
					m_SearchResult.clear();
					m_SearchString.clear();
				}

				m_CurrentNode = &node;
				break;
			}
		}
		ImGui::PopStyleVar();

		ImGui::EndColumns();
		

		ImGui::End();
	}

	void ContentBrowserPanel::Refresh()
	{
		m_AssetDirectory = Project::GetAssetDirectory();
		String currentFilePath = m_CurrentNode != nullptr ? m_CurrentNode->FilePath : String();

		m_TreeRoot = TreeNode();

		m_TreeRoot.IsFolder = true;
		m_TreeRoot.FilePath = m_AssetDirectory.string();
		m_TreeRoot.FileName = FilePath(m_TreeRoot.FilePath).filename().string();
		m_TreeRoot.ParentNode = nullptr;

		ReloadTreeHierarchy(m_AssetDirectory);

		// Update current node

		if (m_CurrentNode == nullptr)
		{
			m_CurrentNode = &m_TreeRoot;
		}
		else
		{
			TreeNode* node = FindTreeNode(m_TreeRoot, currentFilePath);
			m_CurrentNode = node == nullptr ? &m_TreeRoot : node;
		}

		if (!m_SearchString.empty())
			Search();
	}

	void ContentBrowserPanel::ReloadTreeHierarchy(const FilePath& srcDirectory)
	{
		std::queue<TreeNode*> queue;
		queue.push(&m_TreeRoot);

		while (!queue.empty())
		{
			TreeNode* node = queue.front();
			queue.pop();

			for (const auto& dirEntry : std::filesystem::directory_iterator(node->FilePath))
			{
				TreeNode child;
				child.IsFolder = dirEntry.is_directory();
				child.FilePath = dirEntry.path().string();
				child.FileName = dirEntry.path().filename().string();
				child.ParentNode = node;

				node->Children.push_back(child);
			}

			for (auto& child : node->Children)
			{
				if (child.IsFolder)
					queue.push(&child);
			}
		}
	}

	ContentBrowserPanel::TreeNode* ContentBrowserPanel::FindTreeNode(TreeNode& root, const String& path)
	{
		for (auto& child : root.Children)
		{
			if (child.FilePath == path)
			{
				return &child;
			}

			if (path.find(child.FilePath) != String::npos)
			{
				return FindTreeNode(child, path);
			}
		}

		return nullptr;
	}

	void ContentBrowserPanel::Search()
	{
		m_SearchResult.clear();
		std::queue<TreeNode*> searchQueue;

		for (auto& child : m_TreeRoot.Children)
			searchQueue.push(&child);

		while (!searchQueue.empty())
		{
			TreeNode* node = searchQueue.front();

			if (node->FileName.starts_with(m_SearchString))
				m_SearchResult.push_back(node);

			for (auto& child : (*node).Children)
				searchQueue.push(&child);

			searchQueue.pop();
		}

		std::sort(m_SearchResult.begin(), m_SearchResult.end(), [this](TreeNode* left, TreeNode* right)
			{
				uint64 leftDist = Math::Abs((int64)left->FileName.size() - (int64)m_SearchString.size());
				uint64 rightDist = Math::Abs((int64)right->FileName.size() - (int64)m_SearchString.size());

				return leftDist < rightDist;
			});
	}
}
