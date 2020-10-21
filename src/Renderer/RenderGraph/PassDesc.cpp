#include "PassDesc.h"
#include "RenderGraphBuilder.h"
#include "Tether.h"

namespace Renderer
{
	PassDesc::PassDesc(RenderGraphBuilder& builder, Tether& tether, uint32_t id)
		: id(id), name(builder.taskName)
	{
		readResources = std::move(tether.readResources);
		writtenResources = std::move(tether.writtenResources);
		executes.emplace_back(builder.execute);
	}

}
