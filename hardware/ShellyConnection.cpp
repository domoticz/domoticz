#include "stdafx.h"
#include "ShellyConnection.h"
#include "ShellyGen1.h"
#include "ShellyGen2.h"

std::shared_ptr<IShellyConnection> CreateShellyConnection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink, const ShellyCoiotOptions& coiot)
{
	if (node.gen == SHELLY_GEN1)
		return CreateShellyGen1Connection(std::move(node), std::move(credentials), sink, coiot);
	if (ShellyIsRpcGeneration(node.gen))
		return CreateShellyGen2Connection(std::move(node), std::move(credentials), sink);
	return nullptr;
}
