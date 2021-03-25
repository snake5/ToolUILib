
#include "pch.h"
#include <set>


struct Settings
{
	int introInt = 5;
	float otherInt = 7.5f;
	bool extAdvHidden = true;
};

struct SetPropEntry
{
	ui::StringView section;
	ui::StringView group;
	ui::StringView searchStr;
	void(*editFunc)(ui::UIContainer*, Settings&);
};


static const SetPropEntry entries[] =
{
	{ "General", "Intro", "Integer", [](ui::UIContainer* ctx, Settings& S) { ui::imm::PropEditInt(ctx, "Integer", S.introInt); } },
	{ "General", "Misc", "Other", [](ui::UIContainer* ctx, Settings& S) { ui::imm::PropEditFloat(ctx, "Other", S.otherInt); } },
	{ "Extended", "Advanced", "Hidden", [](ui::UIContainer* ctx, Settings& S) { ui::imm::PropEditBool(ctx, "Hidden", S.extAdvHidden); } },
};


struct SettingsWindowDemo : ui::Buildable
{
	static constexpr bool Persistent = true;

	std::string search = "";
	std::string currentSection = "General";
	Settings settings;

	void Build(ui::UIContainer* ctx) override
	{
		auto& sp = ctx->Push<ui::SplitPane>();

		ctx->Push<ui::ListBox>();

		ui::imm::EditString(ctx, search.c_str(), [this](const char* v) { search = v; }, { ui::TextboxPlaceholder("Search") });

		std::unordered_set<ui::StringView> sections;
		for (auto& entry : entries)
		{
			if (!SearchMatch(entry.searchStr))
				continue;
			if (sections.insert(entry.section).second)
			{
				ctx->MakeWithText<ui::Selectable>(entry.section).Init(entry.section == currentSection)
					+ ui::EventHandler(ui::EventType::Activate, [this, &entry](ui::Event&)
				{
					currentSection.assign(entry.section.data(), entry.section.size());
					Rebuild();
				});
			}
		}

		ctx->Pop();

		ctx->PushBox();

		ui::StringView lastGroup;
		for (auto& entry : entries)
		{
			if (entry.section != currentSection)
				continue;
			if (!SearchMatch(entry.searchStr))
				continue;

			if (entry.group != lastGroup)
			{
				if (lastGroup.size())
					ctx->Pop();

				if (entry.group.size())
				{
					ctx->Text(entry.group);
					ctx->Push<ui::ListBox>();
				}
				lastGroup = entry.group;
			}

			entry.editFunc(ctx, settings);
		}
		if (lastGroup.size())
			ctx->Pop();

		ctx->Pop();

		ctx->Pop();
		sp.SetSplits({ 0.4f });
	}

	bool SearchMatch(ui::StringView s)
	{
		if (search.empty())
			return true;

		return s.find_first_at(search) != SIZE_MAX;
	}
};
void Demo_SettingsWindow(ui::UIContainer* ctx)
{
	ctx->Make<SettingsWindowDemo>();
}
