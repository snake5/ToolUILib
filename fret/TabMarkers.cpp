
#include "pch.h"
#include "TabMarkers.h"

#include "Workspace.h"


void TabMarkers::Build()
{
	auto* f = of->ddFile;
	auto& spmkr = ui::Push<ui::SplitPane>();
	{
		ui::PushBox() + ui::SetLayout(ui::layouts::EdgeSlice());

		ui::Text("Marked items") + ui::SetPadding(5);
		auto& tv = ui::Make<ui::TableView>();
		curTable = &tv;
		tv + ui::SetLayout(ui::layouts::EdgeSlice()) + ui::SetHeight(ui::Coord::Percent(100));
		tv.SetDataSource(&f->mdSrc);
		tv.SetSelectionStorage(&f->mdSrc);
		tv.SetSelectionMode(ui::SelectionMode::Single);
		tv.CalculateColumnWidths();
		tv.HandleEvent(&tv, ui::EventType::Click) = [this, f, &tv](ui::Event& e)
		{
			size_t row = tv.GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == ui::MouseButton::Left && e.numRepeats == 2)
			{
				Marker& M = f->markerData.markers[row];
				of->hexViewerState.GoToPos(M.at);
			}
		};
		tv.HandleEvent(&tv, ui::EventType::ContextMenu) = [this, f, &tv](ui::Event& e)
		{
			size_t row = tv.GetHoverRow();
			if (row != SIZE_MAX)
			{
				Marker& M = f->markerData.markers[row];

				auto& CM = ui::ContextMenu::Get();
				CM.Add("Go to start", false, false, 0) = [this, &M]() { of->hexViewerState.GoToPos(M.at); };
				CM.Add("Go to end", false, false, 1) = [this, &M]() { of->hexViewerState.GoToPos(M.GetEnd()); };
			}
		};
		tv.HandleEvent(&tv, ui::EventType::SelectionChange) = [](ui::Event& e) { e.current->Rebuild(); };
		tv.HandleEvent(&tv, ui::EventType::KeyAction) = [&tv, f](ui::Event& e)
		{
			if (e.GetKeyAction() == ui::KeyAction::Delete)
			{
				if (f->mdSrc.selected < f->markerData.markers.size())
				{
					f->markerData.markers.erase(f->markerData.markers.begin() + f->mdSrc.selected);
					f->mdSrc.selected = SIZE_MAX;
					e.current->Rebuild();
				}
			}
		};

		ui::Pop();

		ui::PushBox() + ui::SetLayout(ui::layouts::EdgeSlice());
		if (f->mdSrc.selected < f->markerData.markers.size())
		{
			auto& MIE = ui::Make<MarkedItemEditor>();
			MIE.dataSource = f->dataSource;
			MIE.marker = &f->markerData.markers[f->mdSrc.selected];
		}
		ui::Pop();
	}
	ui::Pop();
	spmkr.SetSplits({ 0.6f });
}
