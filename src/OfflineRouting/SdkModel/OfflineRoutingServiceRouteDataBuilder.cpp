// Copyright eeGeo Ltd (2012-2019), All Rights Reserved

#include "OfflineRoutingServiceRouteDataBuilder.h"
#include "IOfflineRoutingDataRepository.h"
#include "OfflineRoutingFindPathResult.h"
#include "OfflineRoutingGraphNode.h"
#include "OfflineRoutingFeature.h"
#include "LatLongAltitude.h"
#include "VectorMath.h"
#include "RouteData.h"
#include "MathFunc.h"

#include <string>

namespace ExampleApp
{
    namespace OfflineRouting
    {
        namespace SdkModel
        {
            namespace
            {
                const std::string DIRECTION_TYPE_DEPART = "depart";
                const std::string DIRECTION_TYPE_ARRIVE = "arrive";
                const std::string DIRECTION_TYPE_TURN = "turn";
                const std::string DIRECTION_TYPE_NEW_NAME = "new name";

                const double WALKING_SPEED_IN_METER_PER_SECOND = 1.4;
                const double DRIVING_SPEED_IN_METER_PER_SECOND = 10;

                enum DirectionType
                {
                    Depart,
                    Turn,
                    NewName,
                    Arrive
                };

                struct RouteEdge
                {
                    const Eegeo::Space::LatLong startPoint;
                    const Eegeo::Space::LatLong endPoint;
                    const int floorId;
                    const RoutingEngine::OfflineRoutingFeatureId featureId;
                    const double bearing;

                    RouteEdge(const Eegeo::Space::LatLong& edgeStart,
                              const Eegeo::Space::LatLong& edgeEnd,
                              const int edgeFloorId,
                              const RoutingEngine::OfflineRoutingFeatureId edgeFeatureId)
                    : startPoint(edgeStart)
                    , endPoint(edgeEnd)
                    , floorId(edgeFloorId)
                    , featureId(edgeFeatureId)
                    , bearing(edgeStart.BearingToInRadians(edgeEnd.GetLatitude(), edgeEnd.GetLongitude()))
                    {
                    }
                };

                const std::string GetDirectionType(DirectionType directionType)
                {
                    switch (directionType)
                    {
                        case DirectionType::Depart:
                            return DIRECTION_TYPE_DEPART;
                        case DirectionType::Arrive:
                            return DIRECTION_TYPE_ARRIVE;
                        case DirectionType::Turn:
                            return DIRECTION_TYPE_TURN;
                        case DirectionType::NewName:
                            return DIRECTION_TYPE_NEW_NAME;
                        default:
                            return "";
                    }
                }

                const std::string GetDirectionModifier(DirectionType directionType)
                {
                    return "";
                }

                Eegeo::Space::LatLong GetLatLongFromEcef(const Eegeo::dv3& ecefPoint)
                {
                    return Eegeo::Space::LatLong::FromECEF(ecefPoint);
                }

                double GetSpeedForTransportationMode(Eegeo::Routes::Webservice::TransportationMode transportationMode)
                {
                    switch (transportationMode)
                    {
                        case Eegeo::Routes::Webservice::TransportationMode::Driving:
                            return DRIVING_SPEED_IN_METER_PER_SECOND;
                        default:
                            return WALKING_SPEED_IN_METER_PER_SECOND;
                    }
                }

                float Distance(const Eegeo::dv3& a, const Eegeo::dv3& b)
                {
                    return Eegeo::Math::Sqrtf(static_cast<float>(a.SquareDistanceTo(b)));
                }

                std::vector<RouteEdge> BuildEdges(const RoutingEngine::IOfflineRoutingDataRepository& offlineRoutingDataRepository,
                                                  const RoutingEngine::OfflineRoutingPointOnGraph& startPoint,
                                                  const RoutingEngine::OfflineRoutingPointOnGraph& endPoint,
                                                  const std::vector<RoutingEngine::OfflineRoutingGraphNodeId>& pathNodes)
                {
                    std::vector<RouteEdge> routeEdges;

                    //Start node
                    const auto& startNode = offlineRoutingDataRepository.GetGraphNode(pathNodes.front());
                    routeEdges.push_back({GetLatLongFromEcef(startPoint.GetPoint()),
                                          GetLatLongFromEcef(startNode.GetPoint()),
                                          startNode.GetFloorId(),
                                          startNode.GetFeatureId()});

                    for (size_t i = 1; i < pathNodes.size(); i++)
                    {
                        const auto& edgeStart = offlineRoutingDataRepository.GetGraphNode(pathNodes.at(i-1));
                        const auto& edgeEnd = offlineRoutingDataRepository.GetGraphNode(pathNodes.at(i));

                        auto distance = edgeStart.GetPoint().SquareDistanceTo(edgeEnd.GetPoint());

                        if (distance < RoutingEngine::MinimumDistanceInMeters)
                        {
                            continue; //Ignore nodes that are at same position. These are connecting nodes from between two features.
                        }

                        //Since we skipped the transition nodes, these should now match
                        Eegeo_ASSERT(edgeStart.GetFeatureId() == edgeEnd.GetFeatureId());

                        routeEdges.push_back({GetLatLongFromEcef(edgeStart.GetPoint()),
                                              GetLatLongFromEcef(edgeEnd.GetPoint()),
                                              edgeEnd.GetFloorId(),
                                              edgeEnd.GetFeatureId()});
                    }

                    //End node
                    const auto& endNode = offlineRoutingDataRepository.GetGraphNode(pathNodes.back());
                    routeEdges.push_back({GetLatLongFromEcef(endNode.GetPoint()),
                                          GetLatLongFromEcef(endPoint.GetPoint()),
                                          endNode.GetFloorId(),
                                          endNode.GetFeatureId()});

                    return routeEdges;
                }

                Eegeo::Routes::Webservice::RouteStep BuildRouteStep(const std::vector<RouteEdge>& routeEdges,
                                                                    const Eegeo::Routes::Webservice::TransportationMode& transportationMode,
                                                                    const RoutingEngine::IOfflineRoutingDataRepository& offlineRoutingDataRepository,
                                                                    const DirectionType currentDirectionType,
                                                                    DirectionType& nextDirectionType,
                                                                    size_t& pathEdgesIterator)
                {
                    const double speed = GetSpeedForTransportationMode(transportationMode);
                    const auto& stepStartEdge = routeEdges.at(pathEdgesIterator);
                    const auto& stepPosition = stepStartEdge.startPoint;
                    const auto& directionType = GetDirectionType(currentDirectionType);
                    const auto& directionModifier = GetDirectionModifier(currentDirectionType);

                    double bearingBefore = pathEdgesIterator == 0 ? 0 : routeEdges.at(pathEdgesIterator-1).bearing;
                    double bearingAfter = stepStartEdge.bearing;
                    double stepDistance = 0;

                    auto lastProcessedEdge = pathEdgesIterator;

                    std::vector<Eegeo::Space::LatLong> stepPath;

                    stepPath.push_back(stepStartEdge.startPoint);
                    stepPath.push_back(stepStartEdge.endPoint);
                    pathEdgesIterator++;

                    //TODO loop to generate a path
                    while (pathEdgesIterator < routeEdges.size())
                    {
                        const auto& previousEdge = routeEdges.at(pathEdgesIterator-1);
                        const auto& currentEdge = routeEdges.at(pathEdgesIterator);

                        if (previousEdge.featureId != currentEdge.featureId)
                        {
                            const auto& previousFeature = offlineRoutingDataRepository.GetFeature(previousEdge.featureId);
                            const auto& currentFeature = offlineRoutingDataRepository.GetFeature(currentEdge.featureId);

                            if (previousFeature.GetIsMultiFloor() != currentFeature.GetIsMultiFloor())
                            {
                                nextDirectionType = DirectionType::NewName;
                                break;
                            }

                            if (previousFeature.GetName() != currentFeature.GetName())
                            {
                                nextDirectionType = DirectionType::NewName;
                                break;
                            }
                        }

                        stepDistance += Distance(currentEdge.startPoint.ToECEF(), currentEdge.endPoint.ToECEF());
                        stepPath.push_back(currentEdge.endPoint);
                        lastProcessedEdge = pathEdgesIterator;
                        pathEdgesIterator++;
                    }

                    const auto& currentEdge = routeEdges.at(lastProcessedEdge);
                    const auto& stepFeature = offlineRoutingDataRepository.GetFeature(currentEdge.featureId);

                    Eegeo::Routes::Webservice::RouteDirections stepDirections =
                    {
                        stepPosition,
                        directionType,
                        directionModifier,
                        bearingBefore,
                        bearingAfter,
                    };

                    return { stepPath,
                             transportationMode,
                             stepFeature.GetInteriorId().Value(),
                             stepFeature.GetName(),
                             stepDirections,
                             stepDistance / speed,
                             stepDistance,
                             currentEdge.floorId,
                             true,
                             stepFeature.GetIsMultiFloor() };
                }
            }

            OfflineRoutingServiceRouteDataBuilder::OfflineRoutingServiceRouteDataBuilder(const RoutingEngine::IOfflineRoutingDataRepository& offlineRoutingDataRepository)
            : m_offlineRoutingDataRepository(offlineRoutingDataRepository)
            {}

            std::vector<Eegeo::Routes::Webservice::RouteData> OfflineRoutingServiceRouteDataBuilder::BuildRouteData(const std::vector<RoutingEngine::OfflineRoutingFindPathResult>& pathResults,
                                                                                                                    const Eegeo::Routes::Webservice::TransportationMode& transportationMode)
            {
                const double speed = GetSpeedForTransportationMode(transportationMode);
                std::vector<Eegeo::Routes::Webservice::RouteData> routeDataVector;
                double distance = 0;
                std::vector<Eegeo::Routes::Webservice::RouteSection> sections;
                sections.reserve(pathResults.size());

                for (const auto& pathResult : pathResults)
                {
                    distance += pathResult.GetPathDistance();

                    Eegeo::Routes::Webservice::RouteSection section = { BuildRouteSteps(pathResult.GetStartPoint(),
                                                                                        pathResult.GetEndPoint(),
                                                                                        pathResult.GetPathNodes(),
                                                                                        transportationMode),
                                                                        pathResult.GetPathDistance() / speed,
                                                                        pathResult.GetPathDistance() };
                    sections.push_back(section);
                }


                //We only need one route data as we only return one path per query instead of multiple path like the routing service
                Eegeo::Routes::Webservice::RouteData routeData = {sections, distance / speed, distance};
                routeDataVector.push_back(routeData);
                return routeDataVector;
            }

            std::vector<Eegeo::Routes::Webservice::RouteStep> OfflineRoutingServiceRouteDataBuilder::BuildRouteSteps(const RoutingEngine::OfflineRoutingPointOnGraph& startPoint,
                                                                                                                     const RoutingEngine::OfflineRoutingPointOnGraph& endPoint,
                                                                                                                     const std::vector<RoutingEngine::OfflineRoutingGraphNodeId>& pathNodes,
                                                                                                                     const Eegeo::Routes::Webservice::TransportationMode& transportationMode)
            {
                auto pathEdges = BuildEdges(m_offlineRoutingDataRepository, startPoint, endPoint, pathNodes);

                std::vector<Eegeo::Routes::Webservice::RouteStep> routeSteps;
                size_t pathEdgesIterator = 0;
                DirectionType currentDirectionType = DirectionType::Depart;

                while (pathEdgesIterator < pathEdges.size())
                {
                    DirectionType nextDirectionType;
                    Eegeo::Routes::Webservice::RouteStep step = BuildRouteStep(pathEdges,
                                                                               transportationMode,
                                                                               m_offlineRoutingDataRepository,
                                                                               currentDirectionType,
                                                                               nextDirectionType,
                                                                               pathEdgesIterator);
                    routeSteps.push_back(step);
                    currentDirectionType = nextDirectionType;
                }

                Eegeo::Routes::Webservice::RouteDirections endDirections = {
                        GetLatLongFromEcef(endPoint.GetPoint()),
                        DIRECTION_TYPE_ARRIVE,
                        "",
                        routeSteps.back().Directions.BearingAfter,
                        0,
                };

                const auto& endNode = m_offlineRoutingDataRepository.GetGraphNode(pathNodes.back());
                const auto& endFeature = m_offlineRoutingDataRepository.GetFeature(endNode.GetFeatureId());

                std::vector<Eegeo::Space::LatLong> endPath;
                endPath.push_back(GetLatLongFromEcef(endPoint.GetPoint()));
                Eegeo::Routes::Webservice::RouteStep endStep = { endPath,
                                                                 transportationMode,
                                                                 endPoint.GetInteriorId().Value(),
                                                                 endFeature.GetName(),
                                                                 endDirections,
                                                                 0,
                                                                 0,
                                                                 endPoint.GetFloorId(),
                                                                 true,
                                                                 false };
                routeSteps.push_back(endStep);

                return routeSteps;
            }
        }
    }
}
