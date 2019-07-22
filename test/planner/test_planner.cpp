#include <gtest/gtest.h>
#include "../../src/planner/Planner.h"
#include "../../src/planner/search/Edge.h"
#include "../../src/planner/SamplingBasedPlanner.h"
#include "../../src/planner/AStarPlanner.h"
#include <robust_dubins/RobustDubins.h>
extern "C" {
#include "dubins.h"
}

using std::vector;
using std::pair;
using std::cerr;
using std::endl;

TEST(UnitTests, UseRobustDubinsTest) {
    RobustDubins::Problem problemStatement;
    problemStatement.set_minTurningRadius(1);

    // Note: heading is in radians
    problemStatement.set_stateInitial(0,0,0); // (x, y, heading)
    problemStatement.set_stateFinal(10,10,1); // (x, y, heading)
    problemStatement.print(); //

    // run the solver
    RobustDubins::Solver rds;
    rds.set_problemStatement(problemStatement);
//    rds.set_numPts(200); // number of waypoints to return (approx.)
    rds.solve();

    // print results
    rds.print();

    // get waypoints/states of the solution
    std::vector<double> x,y,h; //
    rds.get_optimalWaypointsSetSpacing(x, y, h, 1);
//    rds.get_optimalWaypoints(x,y,h);

    std::cout << '\n';
    for (int i = 0; i < x.size(); i++){
        std::cout << x[i] << ", " << y[i] << ", " << h[i] << '\n';
    }
    std::cout << endl;

    // alternately, get the RobustDubins::Path object
    // (this contains all the information about the optimal solution)
    RobustDubins::Path optimalPath = rds.get_optimalPath();
    optimalPath.print(); // prints info about this object
}

TEST(Benchmarks, RobustDubinsBenchmark1) {
    double minX, maxX, minY, maxY, minSpeed = 2.5, maxSpeed = 2.5;
    double magnitude = 2.5 * TIME_HORIZON;
    minX = -magnitude;
    maxX = magnitude;
    minY = -magnitude;
    maxY = magnitude;
    StateGenerator generator(minX, maxX, minY, maxY, minSpeed, maxSpeed, 7);
    auto root = Vertex::makeRoot(generator.generate(), Path());
    for (int i = 0; i < 100000; i++) {
        auto v = Vertex::connect(root, generator.generate());
        v->parentEdge()->computeApproxCost(maxSpeed, 8);
    }
}

TEST(Benchmarks, RobustDubinsBenchmark2) {
    double minX, maxX, minY, maxY, minSpeed = 2.5, maxSpeed = 2.5;
    double magnitude = 2.5 * TIME_HORIZON;
    minX = -magnitude;
    maxX = magnitude;
    minY = -magnitude;
    maxY = magnitude;
    StateGenerator generator(minX, maxX, minY, maxY, minSpeed, maxSpeed, 7);
    auto start = generator.generate();
    for (int i = 0; i < 100000; i++) {
        auto end = generator.generate();

        RobustDubins::Problem problemStatement;
        problemStatement.set_minTurningRadius(8);

        // Note: heading is in radians
        problemStatement.set_stateInitial(start.x, start.y, start.yaw()); // (x, y, heading)
        problemStatement.set_stateFinal(end.x, end.y, end.yaw()); // (x, y, heading)

        // run the solver
        RobustDubins::Solver rds;
        rds.set_problemStatement(problemStatement);
//    rds.set_numPts(200); // number of waypoints to return (approx.)
        rds.solve();
    }
}

TEST(Benchmarks, DubinsBenchmark1) {
    double minX, maxX, minY, maxY, minSpeed = 2.5, maxSpeed = 2.5;
    double magnitude = 2.5 * TIME_HORIZON;
    minX = -magnitude;
    maxX = magnitude;
    minY = -magnitude;
    maxY = magnitude;
    StateGenerator generator(minX, maxX, minY, maxY, minSpeed, maxSpeed, 7);
    auto start = generator.generate();
    for (int i = 0; i < 100000; i++) {
        auto end = generator.generate();
        double q0[] = {start.x, start.y, start.yaw()};
        double q1[] = {end.x, end.y, end.yaw()};
        DubinsPath dubinsPath;
        dubins_shortest_path(&dubinsPath, q0, q1, 8);
    }
}

TEST(UnitTests, DubinsComparison) {
    double minX, maxX, minY, maxY, minSpeed = 2.5, maxSpeed = 2.5;
    double magnitude = 2.5 * TIME_HORIZON;
    minX = -magnitude;
    maxX = magnitude;
    minY = -magnitude;
    maxY = magnitude;
    StateGenerator generator(minX, maxX, minY, maxY, minSpeed, maxSpeed, 7);
    auto start = generator.generate();
    for (int i = 0; i < 100000; i++) {
        auto end = generator.generate();

        RobustDubins::Problem problemStatement;
        problemStatement.set_minTurningRadius(8);

        // Note: heading is in radians
        problemStatement.set_stateInitial(start.x, start.y, start.yaw()); // (x, y, heading)
        problemStatement.set_stateFinal(end.x, end.y, end.yaw()); // (x, y, heading)

        // run the solver
        RobustDubins::Solver rds;
        rds.set_problemStatement(problemStatement);
//    rds.set_numPts(200); // number of waypoints to return (approx.)
        rds.solve();

        double q0[] = {start.x, start.y, start.yaw()};
        double q1[] = {end.x, end.y, end.yaw()};
        DubinsPath dubinsPath;
        dubins_shortest_path(&dubinsPath, q0, q1, 8);

        EXPECT_LE(fabs(rds.get_optimalPath().get_cost() - dubins_path_length(&dubinsPath)), 0.0001);
    }
}

TEST(UnitTests, DynamicObstacleTest1) {
    DynamicObstaclesManager obstaclesManager;
    obstaclesManager.update(1, State(10, 10, M_PI, 2, 7), .1, 0);
    auto p = obstaclesManager.collisionExists(10, 4, 10);
    EXPECT_DOUBLE_EQ(p, 1);
}

TEST(UnitTests, MakePlanTest) {
    State s1(0, 0, 0, 1, 1);
    State s2(0, 5, 0, 1, 6);
    auto v1 = std::make_shared<Vertex>(s1);
    auto v2 = Vertex::connect(v1, s2);
    auto e = v2->parentEdge();
    auto a = e->computeApproxCost(1, 2);
    EXPECT_DOUBLE_EQ(a, 5);
    auto plan = e->getPlan(1);
    EXPECT_TRUE(plan.getRef().front() == s1);
    EXPECT_GE(plan.getRef().back().y, 4.5); // because of plan density
}

TEST(UnitTests, ComputeEdgeCostTest) {
    State s1(0, 0, 0, 1, 1);
    State s2(0, 5, 0, 1, 0);
    auto v1 = std::make_shared<Vertex>(s1);
    auto v2 = Vertex::connect(v1, s2);
    auto e = v2->parentEdge();
    auto a = e->computeApproxCost(1, 2);
    Map map;
    DynamicObstaclesManager dynamicObstacles;
    Path path;
    path.add(0, 10);
    auto c = e->computeTrueCost(&map, &dynamicObstacles, 1, 2);
    EXPECT_DOUBLE_EQ(6, e->end()->state().time);
    EXPECT_DOUBLE_EQ(c, a);
}

TEST(UnitTests, RunStateGenerationTest) {
    double minX, maxX, minY, maxY, minSpeed = 2.5, maxSpeed = 2.5;
    double magnitude = 2.5 * TIME_HORIZON;
    minX = -magnitude;
    maxX = magnitude;
    minY = -magnitude;
    maxY = magnitude;
    StateGenerator generator(minX, maxX, minY, maxY, minSpeed, maxSpeed, 7); // lucky seed
    while (true) {
        cerr << generator.generate().toString() << endl;
        sleep(1);
        break;
    }
}

TEST(UnitTests, VertexTests1) {
    Path path;
    path.add(50, 50);
    auto root = Vertex::makeRoot(State(5, 5, M_PI, 2.5, 1), path);
    auto v1 = Vertex::connect(root, State(5, -20, M_PI, 2.5, 0));
    auto c = v1->parentEdge()->computeApproxCost(2.5, 8);
    EXPECT_DOUBLE_EQ(c, 10);
    Map m;
    DynamicObstaclesManager obstacles;
    auto t = v1->parentEdge()->computeTrueCost(&m, &obstacles, 2.5, 8);
    EXPECT_DOUBLE_EQ(t, c);
    EXPECT_DOUBLE_EQ(t, v1->currentCost());
    EXPECT_DOUBLE_EQ(v1->currentCost(), v1->state().time - 1);
    auto h = v1->computeApproxToGo();
    EXPECT_DOUBLE_EQ(path.maxDistanceFrom(v1->state()) / 2.5, h);
    EXPECT_DOUBLE_EQ(v1->f(), t + h);
}

TEST(PlannerTests, DubinsWalkTest) {
    // runs forever
    vector<pair<double, double>> points;
    points.emplace_back(10, 10);
    points.emplace_back(20, 10);
    points.emplace_back(20, 20);
    points.emplace_back(10, 20);
    Planner planner(1, 8, Map());
    planner.addToCover(points);
    State start(0,0,M_PI_2,1,1);
    vector<pair<double , double>> newlyCovered;
    while(!points.empty()) {
        if (Path::covers(points.front(), start.x, start.y)) {
            cerr << "Covered a point near " << start.x << ", " << start.y << endl;
            newlyCovered.push_back(points.front());
            points.erase(points.begin());
        }
        auto plan = planner.plan(newlyCovered, start, DynamicObstaclesManager(), 0);
        newlyCovered.clear();
        start = plan[1];
//        cerr << start.toString() << endl;
    }
}

TEST(PlannerTests, PointToPointTest1) {
    vector<pair<double, double>> points;
    points.emplace_back(0, 10);
    points.emplace_back(0, 20);
    points.emplace_back(0, 30);
    Planner planner(2.5, 8, Map());
    planner.addToCover(points);
    State start(0, 0, 0, 2.5, 1);
    auto plan = planner.plan(vector<pair<double, double>>(), start, DynamicObstaclesManager(), 0);
    for (auto s : plan) cerr << s.toString() << endl;
}

TEST(PlannerTests, UCSTest1) {
    vector<pair<double, double>> points;
    points.emplace_back(0, 10);
    points.emplace_back(0, 20);
    points.emplace_back(0, 30);
    SamplingBasedPlanner planner(2.5, 8, Map());
    planner.addToCover(points);
    State start(0, 0, 0, 2.5, 1);
    auto plan = planner.plan(vector<pair<double, double>>(), start, DynamicObstaclesManager(), 0);
    for (auto s : plan) cerr << s.toString() << endl;
}

TEST(PlannerTests, VertexQueueTests) {
    State start(0, 0, 0, 2.5, 1);
    Path path;
    path.add(0, 10);
    path.add(0, 20);
    path.add(0, 30);
    Map m;
    DynamicObstaclesManager obstacles;
    auto root = Vertex::makeRoot(start, path);
    AStarPlanner planner(2.5, 8, m);
    planner.addToCover(path.get());
    planner.pushVertexQueue(root);
    auto popped = planner.popVertexQueue();
    EXPECT_EQ(root, popped);
    State s1(0, 10, 0, 2.5, 0), s2(0, -10, M_PI, 2.5, 0);
    auto v1 = Vertex::connect(root, s1);
    v1->parentEdge()->computeTrueCost(&m, &obstacles, 2.5, 8);
    v1->setCurrentCost();
    auto v2 = Vertex::connect(root, s2);
    v2->parentEdge()->computeTrueCost(&m, &obstacles, 2.5, 8);
    v2->setCurrentCost();
    planner.pushVertexQueue(v1);
    planner.pushVertexQueue(v2);
    popped = planner.popVertexQueue();
    EXPECT_EQ(v1, popped);
}

TEST(UnitTests, EmptyVertexQueueTest) {
    AStarPlanner planner(2.5, 8, Map());
    EXPECT_THROW(planner.popVertexQueue(), std::out_of_range);
}

TEST(UnitTests, ExpandTest1) {
    State start(0, 0, 0, 2.5, 1);
    Path path;
    path.add(0, 10);
    path.add(0, 20);
    path.add(0, 30);
    Map m;
    DynamicObstaclesManager obstacles;
    auto root = Vertex::makeRoot(start, path);
    AStarPlanner planner(2.5, 8, m);
    planner.addToCover(path.get());
    StateGenerator generator(-50, 50, -50, 50, 2.5, 2.5, 7);
    planner.addSamples(generator, 1000);
    planner.expand(root, &obstacles);
    double fPrev = 0;
    for (int i = 0; i < 18; i++) { // k + 1, at the time of this writing
        auto v = planner.popVertexQueue();
//        cerr << "Popped vertex at " << v->state().toString() << endl;
//        cerr << "g = " << v->currentCost() << ", h = " << v->approxToGo() << ", f = " << v->f() << endl;
        EXPECT_LE(fPrev, v->f());
        fPrev = v->f();
    }
    EXPECT_THROW(planner.popVertexQueue(), std::out_of_range);
}

TEST(PlannerTests, RHRSAStarTest1) {
    vector<pair<double, double>> points;
    points.emplace_back(0, 10);
    points.emplace_back(0, 20);
    points.emplace_back(0, 30);
    AStarPlanner planner(2.5, 8, Map());
    planner.addToCover(points);
    State start(0, 0, 0, 2.5, 1);
    auto plan = planner.plan(vector<pair<double, double>>(), start, DynamicObstaclesManager(), 0.95);
    EXPECT_FALSE(plan.empty());
    for (auto s : plan) cerr << s.toString() << endl;
}

TEST(PlannerTests, RHRSAStarTest2) {
    vector<pair<double, double>> points;
    points.emplace_back(0, 10);
    points.emplace_back(0, 20);
    points.emplace_back(0, 30);
    AStarPlanner planner(2.5, 8, Map());
    planner.addToCover(points);
    State start(0, 0, 0, 2.5, 1);
    vector<pair<double , double>> newlyCovered;
    while(!points.empty()) {
        if (Path::covers(points.front(), start.x, start.y)) {
            cerr << "Covered a point near " << start.x << ", " << start.y << endl;
            newlyCovered.push_back(points.front());
            points.erase(points.begin());
        }
        auto plan = planner.plan(newlyCovered, start, DynamicObstaclesManager(), 0.5); // quick iterations
        newlyCovered.clear();
        ASSERT_FALSE(plan.empty());
        start = plan[1];
        ASSERT_LT(start.time, 60);
        cerr << start.toString() << endl;
    }
}

TEST(PlannerTests, RHRSAStarTest3) {
    Path path;
    path.add(10, 10);
    path.add(20, 10);
    path.add(20, 20);
    path.add(10, 20);
    AStarPlanner planner(2.5, 8, Map());
    planner.addToCover(path.get());
    State start(0, 0, 0, 1, 1);
    while(path.size() != 0) {
        cerr << start.toString() << endl;
        auto newlyCovered = path.removeNewlyCovered(start.x, start.y);
        if (!newlyCovered.empty()) {
            cerr << "Covered a point near " << start.x << ", " << start.y << endl;
        }
        auto plan = planner.plan(newlyCovered, start, DynamicObstaclesManager(), 0.95); // quick iterations
        ASSERT_FALSE(plan.empty());
        start = plan[1];
        ASSERT_LT(start.time, 60);
    }
}


int main(int argc, char **argv){
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}