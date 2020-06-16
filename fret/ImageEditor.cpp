
#include "pch.h"
#include "ImageEditor.h"


void ImageEditorWindowNode::Render(UIContainer* ctx)
{
	auto* sp1 = ctx->Push<ui::SplitPane>();
	{
		auto* sp2 = ctx->Push<ui::SplitPane>();
		{
			ctx->Push<ui::Panel>();
			if (ddiSrc.dataDesc && curInst < ddiSrc.dataDesc->instances.size())
			{
				auto* img = ctx->Make<ui::ImageElement>();
				*img + ui::Width(style::Coord::Percent(100));
				*img + ui::Height(style::Coord::Percent(100));
				img->GetStyle().SetPaintFunc([](const style::PaintInfo& info)
				{
					auto bgr = ui::Theme::current->GetImage(ui::ThemeImage::CheckerboardBackground);

					GL::BatchRenderer br;
					auto r = info.rect;

					GL::SetTexture(bgr->_texture);
					br.Begin();
					br.SetColor(1, 1, 1, 1);
					br.Quad(r.x0, r.y0, r.x1, r.y1, 0, 0, r.GetWidth() / bgr->GetWidth(), r.GetHeight() / bgr->GetHeight());
					br.End();
				});
				img->SetImage(cachedImg.GetImage(ddiSrc.dataDesc->GetInstanceImage(*ddiSrc.dataDesc->instances[curInst])));
				img->SetScaleMode(ui::ScaleMode::Fit);
			}
			ctx->Pop();

			ctx->PushBox();
			if (structDef)
			{
				EditImageFormat(ctx, "Format", image->format);
				ctx->Text("Conditional format overrides") + ui::Padding(5);
				ctx->Push<ui::Panel>();
				for (auto& FO : image->formatOverrides)
				{
					EditImageFormat(ctx, "Format", FO.format);
					ctx->Text("Conditions") + ui::Padding(5);
					ctx->Push<ui::Panel>();
					for (size_t i = 0; i < FO.conditions.size(); i++)
					{
						auto& C = FO.conditions[i];
						ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
						ui::imm::PropEditBool(ctx, "\bExpr", C.useExpr);
						if (C.useExpr)
						{
							ui::imm::EditString(ctx, C.expr.expr.c_str(), [&C](const char* v) { C.expr.SetExpr(v); });
						}
						else
						{
							ui::imm::PropEditString(ctx, "\bField", C.field.c_str(), [&C](const char* v) { C.field = v; });
							ui::imm::PropEditString(ctx, "\bValue", C.value.c_str(), [&C](const char* v) { C.value = v; });
						}
						if (ui::imm::Button(ctx, "X", { ui::Width(20) }))
						{
							FO.conditions.erase(FO.conditions.begin() + i);
						}
						ctx->Pop();
					}
					if (ui::imm::Button(ctx, "Add"))
					{
						FO.conditions.push_back({ "unnamed", "" });
					}
					ctx->Pop();
				}
				if (ui::imm::Button(ctx, "Add"))
				{
					image->formatOverrides.push_back({});
				}
				ctx->Pop();
				ui::imm::PropEditString(ctx, "Image offset", image->imgOff.expr.c_str(), [this](const char* v) { image->imgOff.SetExpr(v); });
				ui::imm::PropEditString(ctx, "Palette offset", image->palOff.expr.c_str(), [this](const char* v) { image->palOff.SetExpr(v); });
				ui::imm::PropEditString(ctx, "Width", image->width.expr.c_str(), [this](const char* v) { image->width.SetExpr(v); });
				ui::imm::PropEditString(ctx, "Height", image->height.expr.c_str(), [this](const char* v) { image->height.SetExpr(v); });
			}
			ctx->Pop();
		}
		ctx->Pop();
		sp2->SetDirection(true);
		sp2->SetSplits({ 0.6f });

		ctx->PushBox();
		if (ddiSrc.dataDesc)
		{
			auto* tv = ctx->Make<ui::TableView>();
			*tv + ui::Layout(style::layouts::EdgeSlice()) + ui::Height(style::Coord::Percent(100));
			tv->SetDataSource(&ddiSrc);
			ddiSrc.refilter = true;
			tv->CalculateColumnWidths();
			tv->HandleEvent(UIEventType::SelectionChange) = [this, tv](UIEvent& e)
			{
				auto sel = tv->selection.GetFirstSelection();
				if (tv->IsValidRow(sel))
					ddiSrc.dataDesc->SetCurrentInstance(ddiSrc.dataDesc->instances[ddiSrc._indices[sel]]);
				e.current->RerenderNode();
			};
		}
		ctx->Pop();
	}
	ctx->Pop();
	sp1->SetDirection(false);
	sp1->SetSplits({ 0.6f });
}
