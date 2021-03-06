#pragma once
#include <map>
#include <vector>

#include "implot/implot.h"
#include "imgui/imgui.h"

namespace Renderer
{
#if DEBUG

	namespace ImPlotVar
	{
		struct PlotVarData
		{
			ImGuiID ID;
			std::vector<float> Data;
			std::vector<float> x;
			float lastData;
			int DataInsertIdx;
			int LastFrame;

			PlotVarData() : ID(0), lastData(0), DataInsertIdx(0), LastFrame(-1) {}
		};

		static std::map<ImGuiID, PlotVarData> g_PlotVarsMap;

		void PlotVar(const char* label, float value, float scale_min = FLT_MAX, float scale_max = FLT_MAX, int buffer_size = 120, bool addVal = true, const char* suffix = "")
		{
			ImGui::PushID(label);
			const auto id = ImGui::GetID(label);
			const auto currentFrame = ImGui::GetFrameCount();

			auto& pvd = g_PlotVarsMap[id];

			if (static_cast<int>(pvd.Data.capacity()) != buffer_size)
			{
				pvd.Data.resize(buffer_size);
				pvd.x.resize(buffer_size);
				memset(&pvd.Data[0], 0, sizeof(float) * buffer_size);
				pvd.DataInsertIdx = 0;
				pvd.LastFrame = -1;
			}

			if (pvd.DataInsertIdx == buffer_size) pvd.DataInsertIdx = 0;

			if (addVal)
			{
				pvd.Data[pvd.DataInsertIdx++] = value;
				pvd.lastData = value;
			}


			ImPlot::SetNextPlotLimits(0,buffer_size ,0,40);
			
			if (pvd.LastFrame != currentFrame)
			{
				ImGui::PlotLines("##plot", &pvd.Data[0], buffer_size, pvd.DataInsertIdx, nullptr, scale_min, scale_max, ImVec2(0, 40));
				ImGui::SameLine();
				ImGui::Text("%s\n%-3.4f%s", label, value, suffix);	// Display last value in buffer
				pvd.LastFrame = currentFrame;
			}

			ImGui::PopID();
		}
	}

	void DrawDebugVisualisations(Core* core, FrameInfo& frameInfo, const std::vector<std::unique_ptr<RenderGraph::Pass>>& passes)
	{
		ImGui::NewFrame();

		ImGui::SetNextWindowPos({ 5, 5 });
		ImGui::Begin("Information");

		ImGui::Text("Width: %d, Height: %d, VSync: %s", core->GetSettings()->width, core->GetSettings()->height, core->GetSettings()->vsync ? "True" : "False");
		ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / static_cast<float>(frameInfo.fps)), frameInfo.fps);

		float framesDiff = frameInfo.frameIndex - core->GetSwapchain()->PreviousFrames();
		if (framesDiff >= 5)
		{
			long long timeDiff = frameInfo.time - core->GetSwapchain()->PreviousFPSSample();
			auto frameTime = timeDiff / (framesDiff + FLT_EPSILON);
			ImPlotVar::PlotVar("Frame Times", frameTime, FLT_MAX, FLT_MAX, 144, frameInfo.frameIndex % 7 == 0, "ms");
		}
		else
		{
			ImPlotVar::PlotVar("Frame Times", FLT_MAX, FLT_MAX, FLT_MAX, 144, false, "ms");
		}

		if (ImGui::CollapsingHeader("Allocations")) { core->GetAllocator()->DebugView(); }

		ImGui::End();
	}

#else
	void DrawDebugVisualisations(Core* core, FrameInfo& frameInfo, const std::vector<std::unique_ptr<RenderGraph::Pass>>& passes)
	{
		ImGui::NewFrame();

		ImGui::SetNextWindowPos({ 5, 5 });
		ImGui::Begin("Information");

		ImGui::Text("Width: %d, Height: %d, VSync: %s", core->GetSettings()->width, core->GetSettings()->height, core->GetSettings()->vsync ? "True" : "False");
		ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / static_cast<float>(frameInfo.fps)), frameInfo.fps);

		ImGui::End();
	}
#endif
}
