
#include "pch.h"
#include "TabStructures.h"

#include "Workspace.h"


void TabStructures::Build()
{
	if (workspace->ddiSrc.filterFileFollow && workspace->curOpenedFile < (int)workspace->openedFiles.size())
		workspace->ddiSrc.filterFile = workspace->openedFiles[workspace->curOpenedFile]->ddFile;

	auto& spstr = ui::Push<ui::SplitPane>();
	{
		ui::PushBox() + ui::SetLayout(ui::layouts::EdgeSlice());

		workspace->ddiSrc.Edit();

		ui::Property::Begin();
		ui::Text("Instances") + ui::SetPadding(5);
		if (ui::imm::Button("Expand all instances"))
		{
			workspace->desc.ExpandAllInstances(workspace->ddiSrc.filterFile);
		}
		if (ui::imm::Button("Delete auto-created"))
		{
			workspace->desc.DeleteAllInstances(workspace->ddiSrc.filterFile, workspace->ddiSrc.filterStruct);
		}
		ui::Property::End();

		auto& tv = ui::Make<ui::TableView>();
		curTable = &tv;
		tv + ui::SetLayout(ui::layouts::EdgeSlice()) + ui::SetHeight(ui::Coord::Percent(100));
		tv.SetDataSource(&workspace->ddiSrc);
		tv.SetSelectionStorage(&workspace->ddiSrc);
		tv.SetSelectionMode(ui::SelectionMode::Single);
		workspace->ddiSrc.refilter = true;
		tv.CalculateColumnWidths();
		tv.HandleEvent(ui::EventType::SelectionChange) = [this, &tv](ui::Event& e) { e.current->Rebuild(); };
		tv.HandleEvent(ui::EventType::Click) = [this, &tv](ui::Event& e)
		{
			size_t row = tv.GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == ui::MouseButton::Left && e.numRepeats == 2)
			{
				auto idx = workspace->ddiSrc._indices[row];
				auto* SI = workspace->desc.instances[idx];
				// find tab showing this SI
				OpenedFile* ofile = nullptr;
				int ofid = -1;
				for (auto* of : workspace->openedFiles)
				{
					ofid++;
					if (of->ddFile != SI->file)
						continue;
					ofile = of;
					break;
				}
				// TODO open a new tab if not opened already
				if (ofile)
				{
					workspace->curOpenedFile = ofid;
					ofile->hexViewerState.basePos = SI->off;
					tv.Rebuild();
				}
			}
		};
		tv.HandleEvent(ui::EventType::KeyAction) = [this, &tv](ui::Event& e)
		{
			if (e.GetKeyAction() == ui::KeyAction::Delete)
			{
				if (workspace->desc.curInst)
				{
					workspace->desc.DeleteInstance(workspace->desc.curInst);
					workspace->ddiSrc.refilter = true;
					e.current->Rebuild();
				}
			}
		};

		ui::Pop();
	}
	{
		ui::PushBox() + ui::SetLayout(ui::layouts::EdgeSlice());
		workspace->desc.EditStructuralItems();
		ui::Pop();
	}
	ui::Pop();
	spstr.SetSplits({ 0.6f });
}
