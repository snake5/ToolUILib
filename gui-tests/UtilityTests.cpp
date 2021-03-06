
#include "pch.h"


struct BasicEasingAnimTest : ui::Buildable
{
	BasicEasingAnimTest()
	{
		animPlayer.onAnimUpdate = [this]() { Rebuild(); };
		anim = new ui::AnimEaseLinear("test", 123, 1);
	}
	void Build() override
	{
		ui::Property::Begin("Control");
		if (ui::imm::Button("Play"))
		{
			animPlayer.SetVariable("test", 0);
			animPlayer.PlayAnim(anim);
		}
		if (ui::imm::Button("Stop"))
		{
			animPlayer.StopAnim(anim);
		}
		ui::MakeWithText<ui::Panel>(std::to_string(animPlayer.GetVariable("test")));
		ui::Property::End();
		sliderVal = animPlayer.GetVariable("test");
		ui::Make<ui::Slider>().Init(sliderVal, { 0, 123, 0 });
	}

	ui::AnimPlayer animPlayer;
	ui::AnimPtr anim;
	float sliderVal = 0;
};
void Test_BasicEasingAnim()
{
	ui::Make<BasicEasingAnimTest>();
}


struct ThreadWorkerTest : ui::Buildable
{
	void Build() override
	{
		ui::MakeWithText<ui::Button>("Do it").HandleEvent(ui::EventType::Activate) = [this](ui::Event&)
		{
			wq.Push([this]()
			{
				for (int i = 0; i <= 100; i++)
				{
					if (wq.HasItems() || wq.IsQuitting())
						return;
					ui::Application::PushEvent(this, [this, i]()
					{
						progress = i / 100.0f;
						Rebuild();
					});
#pragma warning (disable:4996)
					_sleep(20);
				}
			}, true);
		};
		auto& pb = ui::MakeWithText<ui::ProgressBar>(progress < 1 ? "Processing..." : "Done");
		pb.progress = progress;
	}

	float progress = 0;

	ui::WorkerQueue wq;
};
void Test_ThreadWorker()
{
	ui::Make<ThreadWorkerTest>();
}


struct ThreadedImageRenderingTest : ui::Buildable
{
	void Build() override
	{
		Subscribe(ui::DCT_ResizeWindow, GetNativeWindow());

		auto& img = ui::Make<ui::ImageElement>();
		img.GetStyle().SetWidth(ui::Coord::Percent(100));
		img.GetStyle().SetHeight(ui::Coord::Percent(100));
		img.SetScaleMode(ui::ScaleMode::Fill);
		img.SetImage(image);

		ui::Application::PushEvent(this, [this, &img]()
		{
			auto cr = img.GetContentRect();
			int tw = cr.GetWidth();
			int th = cr.GetHeight();

			if (image && image->GetWidth() == tw && image->GetHeight() == th)
				return;

			wq.Push([this, tw, th]()
			{
				ui::Canvas canvas(tw, th);

				auto* px = canvas.GetPixels();
				for (uint32_t y = 0; y < th; y++)
				{
					if (wq.HasItems() || wq.IsQuitting())
						return;

					for (uint32_t x = 0; x < tw; x++)
					{
						float res = (((x & 1) + (y & 1)) & 1) ? 1.0f : 0.1f;
						float s = sinf(x * 0.02f) * 0.2f + 0.5f;
						double q = 1 - fabsf(y / float(th) - s) / 0.1f;
						if (q < 0)
							q = 0;
						res *= 1 - q;
						res += 0.5f * q;
						uint8_t c = res * 255;
						px[x + y * tw] = 0xff000000 | (c << 16) | (c << 8) | c;
					}
				}

				ui::Application::PushEvent(this, [this, canvas{ std::move(canvas) }]()
				{
					image = ui::draw::ImageCreateFromCanvas(canvas);
					Rebuild();
				});
			}, true);
		});
	}

	ui::WorkerQueue wq;
	ui::draw::ImageHandle image;
};
void Test_ThreadedImageRendering()
{
	ui::Make<ThreadedImageRenderingTest>();
}


struct OSCommunicationTest : ui::Buildable
{
	OSCommunicationTest()
	{
		animReq.callback = [this]() { Rebuild(); };
		animReq.BeginAnimation();
	}
	void Build() override
	{
		{
			ui::Property::Scope ps("\bClipboard");
			bool hasText = ui::Clipboard::HasText();
			ui::imm::EditBool(hasText, nullptr, { ui::Enable(false) });
			ui::imm::EditString(clipboardData.c_str(), [this](const char* v) { clipboardData = v; });
			if (ui::imm::Button("Read", { ui::SetWidth(ui::Coord::Fraction(0.1f)) }))
				clipboardData = ui::Clipboard::GetText();
			if (ui::imm::Button("Write", { ui::SetWidth(ui::Coord::Fraction(0.1f)) }))
				ui::Clipboard::SetText(clipboardData);
		}

		ui::Textf("time (ms): %u, double click time (ms): %u",
			unsigned(ui::platform::GetTimeMs()),
			unsigned(ui::platform::GetDoubleClickTime())) + ui::SetPadding(5);

		auto pt = ui::platform::GetCursorScreenPos();
		auto col = ui::platform::GetColorAtScreenPos(pt);
		ui::Textf("cursor pos:[%d;%d] color:[%d;%d;%d;%d]",
			pt.x, pt.y,
			col.r, col.g, col.b, col.a) + ui::SetPadding(5);
		ui::Make<ui::ColorInspectBlock>().SetColor(col);

		if (ui::imm::Button("Show error message"))
			ui::platform::ShowErrorMessage("Error", "Message");

		if (ui::imm::Button("Browse to file"))
			ui::platform::BrowseToFile("gui-theme2.tga");
	}

	ui::AnimationCallbackRequester animReq;

	std::string clipboardData;
};
void Test_OSCommunication()
{
	ui::Make<OSCommunicationTest>();
}


struct FileSelectionWindowTest : ui::Buildable
{
	void Build() override
	{
		ui::Text("Check for change");
		ui::imm::PropText("Current working directory", ui::GetWorkingDirectory().c_str());

		ui::Text("Inputs");
		ui::Property::Begin("Filters");
		ui::PushBox();
		{
			auto& se = ui::Make<ui::SequenceEditor>();
			se.SetSequence(Allocate<ui::StdSequence<decltype(fsw.filters)>>(fsw.filters));
			se.itemUICallback = [this](ui::SequenceEditor* se, size_t idx, void* ptr)
			{
				auto* filter = static_cast<ui::FileSelectionWindow::Filter*>(ptr);
				ui::imm::PropEditString("\bName", filter->name.c_str(), [filter](const char* v) { filter->name = v; });
				ui::imm::PropEditString("\bExts", filter->exts.c_str(), [filter](const char* v) { filter->exts = v; });
			};
			if (ui::imm::Button("Add"))
				fsw.filters.push_back({});
		}
		ui::Pop();
		ui::Property::End();

		ui::imm::PropEditString("Default extension", fsw.defaultExt.c_str(), [&](const char* s) { fsw.defaultExt = s; });
		ui::imm::PropEditString("Title", fsw.title.c_str(), [&](const char* s) { fsw.title = s; });
		ui::Property::Begin("Options");
		ui::imm::EditFlag(fsw.flags, unsigned(ui::FileSelectionWindow::MultiSelect), "Multi-select", {}, ui::imm::ButtonStateToggleSkin());
		ui::imm::EditFlag(fsw.flags, unsigned(ui::FileSelectionWindow::CreatePrompt), "Create prompt", {}, ui::imm::ButtonStateToggleSkin());
		ui::Property::End();

		ui::Text("Inputs / outputs");
		ui::imm::PropEditString("Current directory", fsw.currentDir.c_str(), [&](const char* s) { fsw.currentDir = s; });
		ui::Property::Begin("Selected files");
		ui::PushBox();
		{
			auto& se = ui::Make<ui::SequenceEditor>();
			se.SetSequence(Allocate<ui::StdSequence<decltype(fsw.selectedFiles)>>(fsw.selectedFiles));
			se.itemUICallback = [this](ui::SequenceEditor* se, size_t idx, void* ptr)
			{
				auto* file = static_cast<std::string*>(ptr);
				ui::imm::PropEditString("\bFile", file->c_str(), [file](const char* v) { *file = v; });
			};
			if (ui::imm::Button("Add"))
				fsw.selectedFiles.push_back({});
		}
		ui::Pop();
		ui::Property::End();

		ui::Text("Controls");
		ui::Property::Begin("Open file selection window");
		if (ui::imm::Button("Open"))
			Show(false);
		if (ui::imm::Button("Save"))
			Show(true);
		ui::Property::End();

		ui::Text("Outputs");
		ui::imm::PropText("Last returned value", lastRet);
	}

	void Show(bool save)
	{
		lastRet = fsw.Show(save) ? "true" : "false";
	}

	ui::FileSelectionWindow fsw;

	const char* lastRet = "-";
};
void Test_FileSelectionWindow()
{
	ui::Make<FileSelectionWindowTest>();
}

