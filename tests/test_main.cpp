#include <exception>
#include <iostream>

void runSimpleJsonTests();
void runMapTests();
void runMapNavigationTests();
void runSeedServiceTests();
void runRunStatsTests();
void runRunControllerTests();
void runEnemyGroundTests();

void runEncounterDirectorTests();
void runEnemyCombatTests();
void runAudioTests();
void runHudViewModelTests();
void runPlayerDamageTests();

int main() {
    try {
        runSimpleJsonTests();
        runMapTests();
        runMapNavigationTests();
        runSeedServiceTests();
        runRunStatsTests();
        runRunControllerTests();
        runEnemyGroundTests();
        runEncounterDirectorTests();
        runEnemyCombatTests();
        runAudioTests();
        runHudViewModelTests();
        runPlayerDamageTests();
        std::cout << "All Silic2 tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
